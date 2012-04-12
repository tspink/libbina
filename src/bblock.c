#include <bina.h>
#include <malloc.h>

static int mark_leaders(struct bina_context *ctx)
{
	int i, bblock_index;

	/* First instruction is a leader. */
	ctx->instructions[0].basic_block_leader = 1;
	bblock_index = 1;
	
	/* Find jump/call sites and instructions. */
	for (i = 1; i < ctx->nr_instructions; i++) {
		struct bina_instruction *ins = &ctx->instructions[i];
		struct bina_instruction *callsite;
				
		switch(ins->type) {
		case IT_CALL:
		case IT_U_BRANCH:
		case IT_C_BRANCH:
			/* The call site of a branch is the leader of a basic block. */
			callsite = bina_instruction_at(ctx, ins->branch_target_offset);
			if (callsite) {
				ins->branch_target = callsite;
				if (!callsite->basic_block_leader) {
					callsite->basic_block_leader = 1;
					bblock_index++;
				}
			}
			
			/* The instruction after a branch is the leader of a basic block. */
			if (ins->next && !ins->next->basic_block_leader) {
				ins->next->basic_block_leader = 1;
				bblock_index++;
			}
			
			break;
		case IT_RETURN:
			/* The instruction after a return is the leader of basic block. */
			if (ins->next && !ins->next->basic_block_leader) {
				ins->next->basic_block_leader = 1;
				bblock_index++;
			}
			
			break;
		default:
			continue;
		}
	}
	
	return bblock_index;
}

static void add_block_linkage(struct bina_basic_block *parent, struct bina_basic_block *linkage)
{
	parent->successors[parent->nr_successors] = linkage;
	parent->nr_successors++;
	
	linkage->predecessors[linkage->nr_predecessors] = parent;
	linkage->nr_predecessors++;
}

static void create_block_graph(struct bina_context *ctx)
{
	int i, n;

	for (i = 0; i < ctx->nr_instructions; i++) {
		struct bina_instruction *ins = &ctx->instructions[i];
		
		/* If instruction has a jump target, then it must be a jump instruction. */
		if (ins->branch_target) {
			/* The target of this jump is a successive block. */
			add_block_linkage(ins->basic_block, ins->branch_target->basic_block);
			
			/* If this jump is conditional, then a successive block
			 * is the next block. */
			 if (ins->type == IT_C_BRANCH) {
				add_block_linkage(ins->basic_block, ins->basic_block->next);
			 }
		} else {
			/* If it's not a jump target, then we need to see if it's at the
			 * end of a bblock.  If it is, then that bblock's successor is
			 * the following bblock.  Unless it's a ret. */
			 if (ins->type == IT_RETURN)
				continue;
				
			 for (n = 0; n < ctx->nr_basic_blocks; n++) {
				 struct bina_basic_block *block = &ctx->blocks[n];
				 struct bina_instruction *last = &block->instructions[block->nr_instructions - 1];
				 
				 /* It /is/ the last! */
				 if (ins == last) {
					/* Check to see if it's actually the end of the code. */
					if (ins->basic_block->index + 1 >= ctx->nr_basic_blocks)
						continue;

					add_block_linkage(ins->basic_block, ins->basic_block->next);
				}
			}
		}
	}
}

void create_block_descriptors(struct bina_context *ctx)
{
	struct bina_basic_block *bblock = NULL;
	int i, block_index = 0;
	
	/* Create the basic block descriptors. */
	for (i = 0; i < ctx->nr_instructions; i++) {
		struct bina_instruction *ins = &ctx->instructions[i];
		
		if (ins->basic_block_leader) {
			bblock = &ctx->blocks[block_index];
			
			if (block_index > 0) {
				bblock->prev = &ctx->blocks[block_index - 1];
				bblock->prev->next = bblock;
			}
			
			bblock->index = block_index;
			bblock->offset = ins->offset;
			bblock->base = ins->base;
			bblock->size = 0;
	
			bblock->instructions = ins;
			bblock->nr_instructions = 0;
			
			bblock->predecessors = calloc(ctx->nr_basic_blocks, sizeof(*bblock->predecessors));
			
			/* There can only ever be at most two successors. */
			bblock->successors = calloc(2, sizeof(*bblock->predecessors));
			
			block_index++;
		}
		
		ins->basic_block = bblock;
		bblock->nr_instructions++;
		bblock->size += ins->size;
	}
}

int bina_detect_basic_blocks(struct bina_context *ctx)
{
	ctx->nr_basic_blocks = mark_leaders(ctx);
	if (ctx->nr_basic_blocks < 0)
		return ctx->nr_basic_blocks;

	ctx->blocks = calloc(ctx->nr_basic_blocks, sizeof(*ctx->blocks));
	if (!ctx->blocks)
		return -1;
	
	create_block_descriptors(ctx);
	create_block_graph(ctx);
	
	return 0;
}

void bina_destroy_basic_blocks(struct bina_context *ctx)
{
	free(ctx->blocks);
}
