#ifndef __BINA_H__
#define __BINA_H__

struct bina_context;
struct bina_instruction;

struct bina_arch {
	int (*disassemble)(struct bina_context *);
	void (*destroy)(struct bina_context *);
	void (*print_instruction)(struct bina_instruction *);
};

extern const struct bina_arch x86_32_arch;

enum bina_instruction_type {
	IT_U_BRANCH,
	IT_C_BRANCH,
	IT_CALL,
	IT_RETURN,
	IT_COMPARE,
	IT_OTHER,
};

enum bina_operand_type {
	OT_NONE 		= 0,
	OT_REGISTER 	= 1,
	OT_IMMEDIATE	= 2,
	OT_REL_NEAR		= 3,
	OT_REL_FAR		= 4,
	OT_ABSOLUTE		= 5,
	OT_EXPRESSION	= 6,
	OT_OFFSET		= 7,
	OT_OTHER		= 8,
};

enum bina_operand_size {
	SZ_U_8			= 0,
	SZ_U_16			= 1,
	SZ_U_32			= 2,
	SZ_U_64			= 3,
	SZ_U_128		= 4,

	SZ_S_8			= 5,
	SZ_S_16			= 6,
	SZ_S_32			= 7,
	SZ_S_64			= 8,
	SZ_S_128		= 9,

	SZ_NA			= 0xFF,
};

struct bina_register_expr {
	int register_index;
	int displacement;
	int scale;
};

struct bina_operand {
	struct bina_instruction *ins;
	
	enum bina_operand_type type;
	enum bina_operand_size size;
	
	union {
		signed char s8;
		signed short s16;
		signed int s32;
		
		unsigned char u8;
		unsigned short u16;
		unsigned int u32;
		
		int register_index;
		struct bina_register_expr reg_expr;
	} value;
};

#define MAX_OPERANDS	4

struct bina_basic_block;
struct bina_instruction {
	unsigned int index;
	unsigned int offset;
	
	char *base;
	unsigned int size;
	
	enum bina_instruction_type type;
	
	/* Instruction operands. */
	struct bina_operand operands[MAX_OPERANDS];
	unsigned int nr_operands;
	
	/* Branch instruction helpers. */
	struct bina_instruction *branch_target;
	unsigned int branch_target_offset;
	
	/* Basic block helpers. */
	int basic_block_leader;
	struct bina_basic_block *basic_block;
	
	struct bina_context *context;
	void *arch_priv;
	
	/* Instruction Traversal. */
	struct bina_instruction *next;
	struct bina_instruction *prev;
};

struct bina_context {
	const struct bina_arch *arch;
	
	char *base;
	unsigned int size;
	
	struct bina_instruction *instructions;
	unsigned int nr_instructions;
	
	struct bina_basic_block *blocks;
	unsigned int nr_basic_blocks;
};

struct bina_basic_block {
	unsigned int index;
	unsigned int offset;
	char *base;
	unsigned int size;
	
	struct bina_instruction *instructions;
	unsigned int nr_instructions;
	
	struct bina_basic_block **predecessors;
	unsigned int nr_predecessors;
	struct bina_basic_block **successors;
	unsigned int nr_successors;
	
	struct bina_basic_block *next;
	struct bina_basic_block *prev;
};

extern struct bina_context *bina_create(const struct bina_arch *arch, char *base, unsigned int size);
extern void bina_destroy(struct bina_context *ctx);

extern void bina_print_instruction(struct bina_instruction *ins);
extern int bina_detect_basic_blocks(struct bina_context *ctx);
extern void bina_destroy_basic_blocks(struct bina_context *ctx);
extern struct bina_instruction *bina_instruction_at(struct bina_context *ctx, unsigned int offset);

#endif
