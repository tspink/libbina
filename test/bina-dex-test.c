#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <bina.h>
#include "dex.h"

static char *binary_file;

static int create_graph(struct bina_context *ctx)
{
	int i, n;
	FILE *graph;
	
	graph = fopen("./graph.dot", "wt");
	
	fprintf(graph, "digraph g {\n");
	
	for (i = 0; i < ctx->nr_basic_blocks; i++) {
		struct bina_basic_block *block = &ctx->blocks[i];
		
		for (n = 0; n < block->nr_successors; n++) {
			fprintf(graph, "  %d -> %d\n", block->index, block->successors[n]->index);
		}
	}
		
	fprintf(graph, "}\n");
	fclose(graph);
	
	return 0;
}

static int process(char *base, unsigned int size)
{
	struct bina_context *ctx;
	
	ctx = bina_create(&dex_arch, base, size);
	if (!ctx) {
		printf("error: unable to create context\n");
		return -1;
	}
	
	bina_detect_basic_blocks(ctx);
	create_graph(ctx);
	
	bina_analyse_loops(ctx);
	
	bina_destroy(ctx);
	return 0;
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
	
	rc = process(buffer, st.st_size);
	munmap(buffer, st.st_size);
	
	return rc;
}
