#include <bina.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libdis.h>

static void printi(x86_insn_t *insn)
{
	char line[256];
	
	x86_format_insn(insn, line, sizeof(line), att_syntax);
	printf("%04x: %s", insn->offset, line);
}

static void decode_branch(struct bina_instruction *bi, x86_insn_t *ri)
{
	unsigned int target;
	x86_op_t *oper = x86_get_branch_target(ri);
	
	/* Convert relative target to absolute offset. */
	if (oper->type == op_relative_near) {
		target = (unsigned int)(oper->data.sbyte + ri->offset + ri->size);
	} else if (oper->type == op_relative_far) {
		target = (unsigned int)(oper->data.sdword + ri->offset + ri->size);
	} else if (oper->type == op_expression) {
		target = -1;
	} else if (oper->type == op_register) {
		target = -1;
	} else {
		printf("abort\n");
		exit(1);
	}
	
	bi->branch_target_offset = target;
}

static void decode_operand_expression(struct bina_operand *bo, x86_op_t *ro)
{
	/*unsigned int     scale;         /* scale factor * /
    x86_reg_t        index, base;   /* index, base registers * /
        int32_t          disp;          /* displacement * /
        char             disp_sign;     /* is negative? 1/0 * /
        char             disp_size; */
    
    /*printf("expr: ");
    printi(ro->insn);
    printf("\n");
    
    printf("scale=%d, index=%d, base=%d, disp=0x%x\n",
		ro->data.expression.scale,
		ro->data.expression.index.id,
		ro->data.expression.base.id,
		ro->data.expression.disp);*/
    
	return;
}

static void decode_operand_data(struct bina_operand *bo, x86_op_t *ro)
{
	switch(ro->datatype) {
	case op_byte:            /* 1 byte integer */
		if (ro->flags && op_signed) {
			bo->size = SZ_S_8;
			bo->value.s8 = ro->data.sbyte;
		} else {
			bo->size = SZ_U_8;
			bo->value.u8 = ro->data.byte;
		}
		break;
	case op_word:            /* 2 byte integer */
		if (ro->flags && op_signed) {
			bo->size = SZ_S_16;
			bo->value.s16 = ro->data.sword;
		} else {
			bo->size = SZ_U_16;
			bo->value.u16 = ro->data.word;
		}
		break;
	case op_dword:           /* 4 byte integer */
		if (ro->flags && op_signed) {
			bo->size = SZ_S_32;
			bo->value.s32 = ro->data.sdword;
		} else {
			bo->size = SZ_U_32;
			bo->value.u32 = ro->data.dword;
		}
		break;
	case op_qword:           /* 8 byte integer */
		if (ro->flags && op_signed) {
			bo->size = SZ_S_64;
			//bo->value.s8 = ro->data.sbyte;
		} else {
			bo->size = SZ_U_64;
			//bo->value.u8 = ro->data.byte;
		}
		break;
	case op_dqword:          /* 16 byte integer */
		if (ro->flags && op_signed) {
			bo->size = SZ_S_128;
			//bo->value.s8 = ro->data.sbyte;
		} else {
			bo->size = SZ_U_128;
			//bo->value.u8 = ro->data.byte;
		}
		break;
    default:
		bo->size = SZ_NA;
		break;
	}
}

static void decode_operand(struct bina_operand *bo, x86_op_t *ro)
{
	bo->type = ro->type;
	
	switch(bo->type) {
	case OT_REGISTER:
		//bo->value.size = ro->data.reg.size;
		bo->value.register_index = ro->data.reg.id;
		break;
	case OT_EXPRESSION:
		decode_operand_expression(bo, ro);
		break;
	default:
		decode_operand_data(bo, ro);
		break;
	}
}

static void decode_operands(struct bina_instruction *bi, x86_insn_t *ri)
{
	int i;
	
	x86_op_t *op1 = x86_operand_1st(ri);
	x86_op_t *op2 = x86_operand_2nd(ri);
	x86_op_t *op3 = x86_operand_3rd(ri);
	
	bi->nr_operands = 0;
	
	for (i = 0; i < MAX_OPERANDS; i++) {
		bi->operands[i].ins = bi;
	}
	
	if (op1) {
		decode_operand(&bi->operands[0], op1);
		bi->nr_operands++;

		if (op2) {
			decode_operand(&bi->operands[1], op2);
			bi->nr_operands++;
			
			if (op3) {
				decode_operand(&bi->operands[2], op3);
				bi->nr_operands++;
			}
		}
	}
}

static void decode(struct bina_instruction *bi, x86_insn_t *ri)
{
	switch(ri->type) {
	case insn_call:
		bi->type = IT_CALL;
		decode_branch(bi, ri);
		break;
	case insn_jmp:
		bi->type = IT_U_BRANCH;
		decode_branch(bi, ri);
		break;
	case insn_jcc:
		bi->type = IT_C_BRANCH;
		decode_branch(bi, ri);
		break;
	case insn_return:
		bi->type = IT_RETURN;
		break;
	case insn_cmp:
		bi->type = IT_COMPARE;
		break;
	default:
		bi->type = IT_OTHER;
		break;
	}
	
	decode_operands(bi, ri);
}

static int x86_32_disasm(struct bina_context *ctx)
{
	int offset, index, length;
	
	ctx->instructions = calloc(ctx->size, sizeof(*ctx->instructions));
	if (!ctx->instructions)
		return -1;
	
	x86_init(opt_none, NULL, NULL);

	index = 0;
	offset = 0;
	while (offset < ctx->size) {
		x86_insn_t insn;
		
		length = x86_disasm((unsigned char *)ctx->base, ctx->size, 0, offset, &insn);

		if (length) {
			struct bina_instruction *bi = &ctx->instructions[index];
			
			bi->index = index;
			bi->offset = offset;
			bi->base = ctx->base + offset;
			bi->size = length;
			bi->context = ctx;
			bi->arch_priv = calloc(1, 256);
			
			if (bi->arch_priv) {
				x86_format_insn(&insn, (char *)bi->arch_priv, 255, att_syntax);
			}
			
			if (index < sizeof(*ctx->instructions) - 1)
				bi->next = &ctx->instructions[index + 1];
			
			if (index > 0)
				bi->prev = &ctx->instructions[index - 1];
			
			decode(bi, &insn);
	
			offset += length;
			index++;
		} else {
			printf("warn: invalid instruction\n");
			offset++;
		}
		
		x86_oplist_free(&insn);
	}

	x86_cleanup();
	
	ctx->nr_instructions = index;
	
	return 0;
}

static void x86_32_destroy(struct bina_context *ctx)
{
	int i;
	for (i = 0; i < ctx->nr_instructions; i++) {
		free(ctx->instructions[i].arch_priv);
	}
	
	free(ctx->instructions);
}

static void x86_32_print(struct bina_instruction *ins)
{
	printf("%s", (char *)ins->arch_priv);
}

const struct bina_arch x86_32_arch = {
	.disassemble = x86_32_disasm,
	.destroy = x86_32_destroy,
	.print_instruction = x86_32_print,
};
