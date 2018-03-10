#ifndef _ANIMAL_H

#define _ANIMAL_H

#include "chemicals.h"
#include <stddef.h>
#include <stdint.h>

struct brain {
	size_t refcount;
	uint16_t ram_size, code_size;
	uint16_t code[];
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

enum opcode {
/* General */
	OP_MOVE,	/* dest src */
	OP_XCHG,	/* dest1 dest2 */
	OP_FLAG,	/* dest */
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
	OP_MUL,		/* dest src */
	OP_SMUL,	/* dest src */
	OP_DIV,		/* dest src */
	OP_SDIV,	/* dest src */
	OP_MOD,		/* dest src */
	OP_SMOD,	/* dest src */
/* Control flow */
	OP_JUMP,	/* dest */
	OP_CMPR,	/* cmp1 cmp2 */
	OP_TEST,	/* test1 test2 */
	OP_JMPZ,	/* dest */
	OP_JPNZ,	/* dest */
	OP_JMPG,	/* dest */
	OP_JPGE,	/* dest */
	OP_JMPL,	/* dest */
	OP_JPLE,	/* dest */
/* Special */
	OP_FACE,	/* direction */
	OP_PICK,	/* id amount */
	OP_DROP,	/* id amount */
	OP_CONV,	/* id1 id2 */
	OP_EAT,		/* id */
	OP_CHEM,	/* dest id */
	OP_BABY,	/* */
};

#endif /* Header guard */
