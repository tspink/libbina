#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libelf.h>
#include <gelf.h>
#include <string.h>
#include <bina.h>
#include <sys/ptrace.h>
#include <sys/user.h>

static char *binary_file;

static int create_graph(struct bina_context *ctx)
{
	int i, n;
	FILE *graph;
	
	graph = fopen("./graph.dot", "wt");
	
	fprintf(graph, "digraph g {\n");
	
	for (i = 0; i < ctx->nr_basic_blocks; i++) {
		struct bina_basic_block *block = &ctx->blocks[i];
		
		/*printf("==== block %d\n", block->index);
		for (n = 0; n < block->nr_instructions; n++) {
			printf("%04x ", block->instructions[n].offset);
			bina_print_instruction(&block->instructions[n]);
			printf("\n");
		}*/
		
		for (n = 0; n < block->nr_successors; n++) {
			fprintf(graph, "  %d -> %d\n", block->index, block->successors[n]->index);
		}
	}
		
	fprintf(graph, "}\n");
	fclose(graph);
	
	return 0;
}

static int break_handler(struct bina_breakpoint *breakpoint)
{
	int new_block = breakpoint->instruction->basic_block->index;
	printf("block %d\n", new_block);
	
	return 0;
}

static int process(char *base, unsigned int size, void *text_base)
{
	struct bina_context *ctx;
	struct bina_trace *trace;
	FILE *graph;
	int i;
	
	ctx = bina_create(&x86_32_arch, base, size);
	if (!ctx) {
		printf("error: unable to create context\n");
		return -1;
	}
	
	bina_detect_basic_blocks(ctx);
	create_graph(ctx);
	
	bina_analyse_loops(ctx);
	
	printf("starting trace\n");
	
	trace = bina_trace_init(ctx, binary_file, text_base, break_handler);
	if (!trace) {
		bina_destroy(ctx);
		printf("error: couldn't setup trace.\n");
		return -1;
	}
	
	graph = fopen("./trace.dot", "wt");
	fprintf(graph, "digraph g {\n");

	
	for (i = 0; i < ctx->nr_basic_blocks; i++) {
		bina_install_breakpoint(trace, ctx->blocks[i].instructions, graph);
	}
	
	bina_trace_run(trace);
	
	fprintf(graph, "}\n");
	fclose(graph);
	
	printf("trace complete\n");
	
	bina_trace_destroy(trace);
	bina_destroy(ctx);
	return 0;
}

static int process_section(GElf_Shdr *hdr, Elf_Scn *scn)
{
	Elf_Data *edata = NULL;
	edata = elf_getdata(scn, edata);
	if (!edata) {
		return -1;
	}
		
	return process(edata->d_buf, edata->d_size, (void *)hdr->sh_addr);
}

static int process_elf(char *base, unsigned int size)
{
	Elf *elf;
	GElf_Shdr section_hdr;
	Elf_Scn *section = NULL;
	Elf32_Ehdr *elf32header = (Elf32_Ehdr *)base;
	char *name;
	int rc = -1;
	
	/* Check the ELF library version. */
	if (elf_version(EV_CURRENT) == EV_NONE) {
		printf("warning: elf library is out of date\n");
	}
	
	/* Open up the binary as an ELF file. */
	elf = elf_memory(base, size);
	if (!elf) {
		printf("error: couldn't open elf file\n");
		return -1;
	}

	/* Loop through the sections looking for the .text section. */
	while ((section = elf_nextscn(elf, section)) != 0) {
		gelf_getshdr(section, &section_hdr);
		name = elf_strptr(elf, elf32header->e_shstrndx, section_hdr.sh_name);
		
		/* If we've found the .text section, process it. */
		if (strcmp(name, ".text") == 0) {
			rc = process_section(&section_hdr, section);
			break;
		}
	}
	
	elf_end(elf);
	return rc;
}

static void usage(char *progname)
{
	printf("usage: %s <binary>\n", progname);
}

int main(int argc, char **argv)
{
	struct stat st;
	char *buffer;
	int fd, rc;

	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}
	
	binary_file = argv[1];
	fd = open(binary_file, O_RDONLY);
	if (fd < 0) {
		printf("error: unable to open file\n");
		return -1;
	}
	
	rc = fstat(fd, &st);
	if (rc) {
		close(fd);
		printf("error: unable to stat file\n");
		return -1;
	}
	
	buffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	
	if (!buffer) {
		printf("error: unable to map file\n");
		return -1;
	}
	
	rc = process_elf(buffer, st.st_size);
	munmap(buffer, st.st_size);
	
	return rc;
}
