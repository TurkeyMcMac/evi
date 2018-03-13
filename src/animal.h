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
	OP_JPTA,	/* dest test */
	OP_JTNA,	/* dest test */
	OP_JPTO,	/* dest test */
	OP_JTNO,	/* dest test */
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
	FGREATER = 1 << 1,
	FLESSER = 1 << 2,
	FINVAL_ARG = 1 << 3,
	FROOB = 1 << 4,
	FCOOB = 1 << 5,
	FINVAL_OPCODE = 1 << 6,
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
