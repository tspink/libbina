#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <bina.h>

int start_child(struct bina_trace *trace)
{
	trace->pid = fork();
	
	if (trace->pid == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execl(trace->path, trace->path, NULL);
	} else if (trace->pid > 0) {
		wait(NULL);
	} else {
		return (int)trace->pid;
	}
	
	return 0;
}

struct bina_trace *bina_trace_init(struct bina_context *ctx, const char *path, void *text_base, bina_break_handler_fn handler)
{
	struct bina_trace *trace;
	int rc;
	
	trace = calloc(1, sizeof(*trace));
	if (!trace)
		return NULL;
		
	trace->context = ctx;
	trace->handler = handler;
	trace->path = path;
	trace->text_base = text_base;
	
	rc = start_child(trace);
	if (rc) {
		free(trace);
		trace = NULL;
	}
		
	return trace;
}

void bina_trace_destroy(struct bina_trace *trace)
{
	if (trace->pid > 0)
		ptrace(PTRACE_KILL, trace->pid, NULL, NULL);
	free(trace);
}

static int do_install(struct bina_breakpoint *brk)
{
	unsigned long address = ((unsigned long)brk->trace->text_base) + (unsigned long)brk->instruction->offset;
	unsigned long word = ptrace(PTRACE_PEEKTEXT, brk->trace->pid, (void *)address, NULL);
	
	((char *)&word)[0] = 0xcc;
	
	ptrace(PTRACE_POKETEXT, brk->trace->pid, (void *)address, word);
	
	return 0;
}

static int do_uninstall(struct bina_breakpoint *brk)
{
	unsigned long address = ((unsigned long)brk->trace->text_base) + (unsigned long)brk->instruction->offset;
	unsigned long word = ptrace(PTRACE_PEEKTEXT, brk->trace->pid, (void *)address, NULL);
	
	((char *)&word)[0] = ((char *)brk->instruction->base)[0];
	
	ptrace(PTRACE_POKETEXT, brk->trace->pid, (void *)address, word);
	
	return 0;
}

struct bina_breakpoint *bina_install_breakpoint(struct bina_trace *trace, struct bina_instruction *ins, void *state)
{
	struct bina_breakpoint *brk;
	int rc;
	
	brk = calloc(1, sizeof(*brk));
	if (!brk)
		return NULL;
		
	brk->trace = trace;
	brk->instruction = ins;
	brk->state = state;
	
	rc = do_install(brk);
	if (rc) {
		free(brk);
		brk = NULL;
	}
	
	trace->breakpoints[trace->nr_breakpoints] = brk;
	trace->nr_breakpoints++;
	
	return brk;
}

struct bina_breakpoint *find_breakpoint(struct bina_trace *trace, unsigned long addr)
{
	unsigned long offset = addr - (unsigned long)trace->text_base;
	int i;
	
	for (i = 0; i < trace->nr_breakpoints; i++) {
		if (trace->breakpoints[i]->instruction->offset == offset) {
			return trace->breakpoints[i];
		}
	}
	
	return NULL;
}

void handle_breakpoint(struct bina_trace *trace)
{
	struct bina_breakpoint *brk;
	struct user_regs_struct regs;

	/* Read the child registers. */
	ptrace(PTRACE_GETREGS, trace->pid, NULL, &regs);
	
	/* Find the breakpoint descriptor, based on where we've stopped. */
	brk = find_breakpoint(trace, regs.eip - 1);
	if (!brk) {
		printf("hmm, we've discovered an unregistered breakpoint\n");
		return;
	}
	
	/* Call breakpoint handler. */
	trace->handler(brk);
	
	/* Now, we need to continue.  To do that, we need to:
	 *   1. Uninstall the breakpoint.
	 *   2. Reset EIP to the location of the breakpoint instruction.
	 *   3. Single step across the instruction.
	 *   4. Put the breakpoint back.
	 */
	 
	/* Step 1: Uninstall. */
	do_uninstall(brk);
	
	/* Step 2: Reset EIP. */
	regs.eip--;
	ptrace(PTRACE_SETREGS, trace->pid, NULL, &regs);
	
	/* Step 3: Single step. */
	ptrace(PTRACE_SINGLESTEP, trace->pid, NULL, NULL);
	wait(NULL);
	
	/* Step 4: Reinstall. */
	do_install(brk);
	
	/* Continue execution of the child. */
	ptrace(PTRACE_CONT, trace->pid, NULL, NULL);
}

int bina_trace_run(struct bina_trace *trace)
{
	int status;
	siginfo_t signal;

	ptrace(PTRACE_CONT, trace->pid, NULL, NULL);
	
	do {
		wait(&status);
		
		if (WIFEXITED(status)) {
			return 0;
		}
		
		ptrace(PTRACE_GETSIGINFO, trace->pid, NULL, &signal);
		
		if (signal.si_signo == SIGTRAP) {
			handle_breakpoint(trace);
		}
	} while(1);
	
	return 0;
}
