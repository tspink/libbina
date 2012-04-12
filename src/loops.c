#include <bina.h>
#include <stdio.h>

static int process_for_loop(struct bina_basic_block *start, struct bina_basic_block *body, struct bina_basic_block *condition)
{
	struct bina_instruction *cmp = condition->instructions;
	
	printf("loop start block: %d\n", start->index);
	printf("loop body block: %d\n", body->index);
	printf("loop test block: %d\n", condition->index);
	bina_print_instruction(cmp);
	printf("\n");
	bina_print_instruction(cmp->next);
	printf("\n");
	
	if (cmp->type != IT_COMPARE) {
		printf("loop test isn't a comparison - rejecting premise\n");
		return -1;
	}
	
	printf("%d %d\n", cmp->operands[0].type, cmp->operands[1].type);
	printf("so, the upper bound might be: %d\n", cmp->operands[1].value.u32);
	
	return 0;
}

static int find_for_loops(struct bina_context *ctx)
{
	int i;
	
	for (i = 0; i < ctx->nr_basic_blocks; i++) {
		struct bina_basic_block *block = &ctx->blocks[i];
		
		/* Not interested in conditional jumps. */
		if (block->nr_successors != 1)
			continue;
		
		/* Not interested in contigous blocks. */
		if (block->successors[0] == block->next)
			continue;
		
		/* Okay, we've got an interesting block. Check to see if the
		 * successive block's successor is this block's next
		 * block. :-O */
		 
		 /* No successor?  Not interested. */
		 if (block->successors[0]->nr_successors == 0) {
			 continue;
		 }
		 
		 if (block->successors[0]->successors[0] == block->next) {
			 printf("i think we've found a for-loop\n");
			 process_for_loop(block, block->next, block->successors[0]);
		 }
	}
	
	return 0;
}

int bina_analyse_loops(struct bina_context *ctx)
{
	return find_for_loops(ctx);
}
