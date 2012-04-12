#include <bina.h>
#include <malloc.h>

struct bina_context *bina_create(const struct bina_arch *arch, char *base, unsigned int size)
{
	struct bina_context *ctx;
	int rc;
	
	if (arch == NULL) {
		return NULL;
	}

	if (base == NULL) {
		return NULL;
	}
	
	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		return NULL;
	}
	
	ctx->base = base;
	ctx->size = size;
	ctx->arch = arch;
	
	rc = arch->disassemble(ctx);
	if (rc) {
		free(ctx);
		return NULL;
	}

	return ctx;
}

void bina_destroy(struct bina_context *ctx)
{
	if (ctx->blocks)
		bina_destroy_basic_blocks(ctx);

	if (ctx->arch->destroy)
		ctx->arch->destroy(ctx);
		
	free(ctx);
}

struct bina_instruction *bina_instruction_at(struct bina_context *ctx, unsigned int offset)
{
	int i;
	
	for (i = 0; i < ctx->nr_instructions; i++) {
		if (ctx->instructions[i].offset == offset) {
			return &ctx->instructions[i];
		}
	}
	
	return NULL;
}

void bina_print_instruction(struct bina_instruction *ins)
{
	ins->context->arch->print_instruction(ins);
}
