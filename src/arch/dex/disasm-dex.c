#include <bina.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int dex_disasm(struct bina_context *ctx)
{
	ctx->instructions = calloc(ctx->size, sizeof(*ctx->instructions));
	if (!ctx->instructions)
		return -1;
	
	ctx->nr_instructions = 0;
	return 0;
}

static void dex_destroy(struct bina_context *ctx)
{
	int i;
	for (i = 0; i < ctx->nr_instructions; i++) {
		free(ctx->instructions[i].arch_priv);
	}
	
	free(ctx->instructions);
}

static void dex_print(struct bina_instruction *ins)
{
	printf("%s", (char *)ins->arch_priv);
}

const struct bina_arch dex_arch = {
	.disassemble = dex_disasm,
	.destroy = dex_destroy,
	.print_instruction = dex_print,
};
