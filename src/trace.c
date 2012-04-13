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
		
		trace->text_base = (void *)0x8048360;
		
		//(void *)ptrace(PTRACE_PEEKUSER, trace->pid, offsetof(struct user,start_code));
		printf("text base: 0x%x\n", (unsigned int)trace->text_base);
	} else {
		return (int)trace->pid;
	}
	
	return 0;
}

struct bina_trace *bina_trace_init(struct bina_context *ctx, const char *path, bina_break_handler_fn handler)
{
	struct bina_trace *trace;
	int rc;
	
	trace = calloc(1, sizeof(*trace));
	if (!trace)
		return NULL;
		
	trace->context = ctx;
	trace->handler = handler;
	trace->path = path;
	
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

static inline int do_install(struct bina_breakpoint *brk)
{
	ptrace(PTRACE_POKETEXT, brk->trace->pid, (void *)brk->addr, brk->code_break);
	return 0;
}

static inline int do_uninstall(struct bina_breakpoint *brk)
{
	ptrace(PTRACE_POKETEXT, brk->trace->pid, (void *)brk->addr, brk->code_real);
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
	
	/* Calculate the real memory address to insert the breakpoint code. */
	brk->addr = ((unsigned long)trace->text_base) + (unsigned long)ins->offset;
	
	/* Read in the original code at the breakpoint location. */
	brk->code_real = ptrace(PTRACE_PEEKTEXT, trace->pid, (void *)brk->addr, NULL);
	
	/* Not too scary, but, generate the break opcode. */
	brk->code_break =
		(brk->code_real & ~trace->context->arch->break_mask) | 
		(trace->context->arch->break_code & trace->context->arch->break_mask);
	
	/* Install the breakpoint. */
	rc = do_install(brk);
	if (rc) {
		free(brk);
		brk = NULL;
	}
	
	/* Store the breakpoint descriptor. */
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

int handle_breakpoint(struct bina_trace *trace)
{
	int break_size = trace->context->arch->break_size;
	struct bina_breakpoint *brk;
	struct user_regs_struct regs;

	/* Read the child registers, and set the instruction pointer
	 * to the location where the breakpoint occurred. */
	ptrace(PTRACE_GETREGS, trace->pid, NULL, &regs);
	regs.eip -= break_size;
	ptrace(PTRACE_SETREGS, trace->pid, NULL, &regs);

	/* Find the breakpoint descriptor, based on where we've stopped. */
	brk = find_breakpoint(trace, regs.eip);
	if (!brk) {
		printf("error: unregistered breakpoint hit\n");
		return -1;
	}
	
	/* Call user-defined breakpoint handler. */
	trace->handler(brk);
	 
	/* Step 1: Uninstall the breakpoint, to reassert the original
	 * code. */
	do_uninstall(brk);
	
	/* Step 2: Single step through the real instruction, and wait for
	 * that to complete. */
	ptrace(PTRACE_SINGLESTEP, trace->pid, NULL, NULL);
	wait(NULL);
	
	/* Step 3: Reinstall the breakpoint, so it continues to get hit. */
	do_install(brk);
	
	/* Continue execution of the child. */
	ptrace(PTRACE_CONT, trace->pid, NULL, NULL);
	
	return 0;
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
		
		/* If we stopped because of a SIGTRAP, then we more than likely
		 * hit a breakpoint.  So, pass off handling the breakpoint to
		 * the handler routine. */
		if (signal.si_signo == SIGTRAP) {
			status = handle_breakpoint(trace);
			
			/* If for some reason we couldn't handle the breakpoint,
			 * then we probably can't continue because we've corrupted
			 * the memory space by messing around with inserting
			 * breakpoint opcodes. */
			if (status)
				return status;
		}
	} while(1);
	
	return 0;
}
