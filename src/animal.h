#ifndef _ANIMAL_H

#define _ANIMAL_H

#include "chemicals.h"
#include <stddef.h>
#include <stdint.h>


enum opcode {
/* General */
	OP_MOVE,	/* dest src */
	OP_XCHG,	/* dest1 dest2 */
	OP_GFLG,	/* dest */
	OP_SFLG,	/* src */
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
/* Control flow */
	OP_JUMP,	/* dest */
	OP_CMPR,	/* cmp1 cmp2 */
	OP_JMPA,	/* dest test */
	OP_JPNA,	/* dest test */
	OP_JMPO,	/* dest test */
	OP_JPNO,	/* dest test */
/* Special */
	OP_FACE,	/* direction */
	OP_PICK,	/* id amount */
	OP_DROP,	/* id amount */
	OP_CONV,	/* id1 id2 */
	OP_EAT,		/* id */
	OP_CHEM,	/* dest id */
	OP_BABY,	/* */

	N_OPCODES
};

struct instruction {
	uint8_t opcode;
	uint8_t l_fmt : 2,
		r_fmt : 2;
	uint16_t left, right;
};
struct brain {
	size_t refcount;
	uint16_t ram_size, code_size;
	struct instruction code[];
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
};

struct animal {
	struct animal *next;
	struct brain *brain;
	size_t x, y;
	uint8_t action, direction;
	uint16_t health;
	uint16_t energy;
	uint16_t lifetime;
	uint16_t instr_ptr;
	uint16_t flags;
	uint8_t stomach[N_CHEMICALS];
	uint16_t ram[];
};

void test_asm(void);

#endif /* Header guard */
