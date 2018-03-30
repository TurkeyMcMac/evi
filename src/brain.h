#ifndef _BRAIN_H

#define _BRAIN_H

#include <stddef.h>
#include <stdint.h>

#define GNRG_COST 1

#define SLLIST_FOR_EACH(list, item) for ((item) = (list); (item) != NULL; (item) = (item)->next)

struct instruction {
	uint8_t opcode;
	uint8_t l_fmt : 2,
		r_fmt : 2;
	uint16_t left, right;
};

struct brain {
	struct brain *next;
	size_t refcount;
	uint32_t save_num;
	uint16_t signature;
	uint16_t ram_size, code_size;
	struct instruction code[];
};

struct brain *brain_new(uint16_t signature, uint16_t ram_size, uint16_t code_size);

enum {
	MKIND_SIGNATURE,
	MKIND_OPCODE,
	MKIND_INSTR_LFMT,
	MKIND_INSTR_RFMT,
	MKIND_INSTR_LEFT,
	MKIND_INSTR_RIGHT,
	MKIND_ADD,
	MKIND_REMOVE_ONE,
	MKIND_REMOVE_MANY,
	MKIND_DUP_ONE,
	MKIND_DUP_MANY,
	MKIND_SWAP_ONE,
	MKIND_SWAP_MANY,
	MKIND_REPLACE,
	MKIND_RAM_SIZE,

	N_MKIND
};

struct grid;

struct brain *brain_mutate(struct brain *self, struct grid *g);

enum opcode {
/* General */
	OP_MOVE,	/* dest src */
	OP_XCHG,	/* dest1 dest2 */
	OP_GFLG,	/* dest */
	OP_SFLG,	/* src */
	OP_GIPT,	/* dest */
/* Bitwise */
	OP_AND,		/* dest src */
	OP_OR,		/* dest src */
	OP_XOR,		/* dest src */
	OP_NOT,		/* flip */
	OP_SHFR,	/* dest amount */
	OP_SHFL,	/* dest amount */
/* Arithmetic */
	OP_ADD,		/* dest src */
	OP_SUB,		/* dest src */
	OP_INCR,	/* increment */
	OP_DECR,	/* decrement */
/* Control flow */
	OP_JUMP,	/* dest */
	OP_CMPR,	/* cmp1 cmp2 */
	OP_JMPA,	/* dest test */
	OP_JPNA,	/* dest test */
	OP_JMPO,	/* dest test */
	OP_JPNO,	/* dest test */
/* Special */
	OP_PICK,	/* direction num:8,id:8 */
	OP_DROP,	/* direction num:8,id:8 */
	OP_LCHM,	/* dest id:6,x:5,y:5 */
	OP_LNML,	/* dest _:6,x:5,y:5 */
	OP_BABY,	/* direction energy */
	OP_STEP,	/* direction */
	OP_ATTK,	/* direction power */
	OP_CONV,	/* id1 id2 */
	OP_EAT,		/* id num */
	OP_GCHM,	/* dest id */
	OP_GHLT,	/* dest */
	OP_GNRG,	/* dest */

	N_OPCODES
};

enum flags {
	FEQUAL = 1 << 0,
	FUGREATER = 1 << 1,
	FULESSER = 1 << 2,
	FSGREATER = 1 << 3,
	FSLESSER = 1 << 4,
	FINVAL_ARG = 1 << 5,
	FROOB = 1 << 6,
	FCOOB = 1 << 7,
	FINVAL_OPCODE = 1 << 8,
	FEMPTY = 1 << 9,
	FFULL = 1 << 10,
	FBLOCKED = 1 << 11,
};

extern const struct opcode_info {
	char name[5];
	uint16_t energy;
} op_info[N_OPCODES];

#endif /* Header guard */
