#include "brain.h"

#include "grid.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define OP_INFO(name, energy) [OP_##name] = {#name, energy}

const struct opcode_info op_info[N_OPCODES] = {
	/*	Name	Energy */
	OP_INFO(MOVE,	     1),
	OP_INFO(XCHG,	     2),
	OP_INFO(GFLG,	     1),
	OP_INFO(SFLG,	     1),
	OP_INFO(GIPT,	     1),
	OP_INFO(AND,	     4),
	OP_INFO(OR,	     4),
	OP_INFO(XOR,	     4),
	OP_INFO(NOT,	     3),
	OP_INFO(SHFR,	     4),
	OP_INFO(SHFL,	     4),
	OP_INFO(ADD,	     6),
	OP_INFO(SUB,	     6),
	OP_INFO(INCR,	     4),
	OP_INFO(DECR,	     4),
	OP_INFO(JUMP,	     4),
	OP_INFO(CMPR,	     6),
	OP_INFO(JMPA,	     6),
	OP_INFO(JPNA,	     6),
	OP_INFO(JMPO,	     6),
	OP_INFO(JPNO,	     6),
	OP_INFO(PICK,	     8),
	OP_INFO(DROP,	     7),
	OP_INFO(LCHM,	     7),
	OP_INFO(LNML,	     7),
	OP_INFO(BABY,	     5),
	OP_INFO(STEP,	     7),
	OP_INFO(ATTK,	     7),
	OP_INFO(CONV,	     6),
	OP_INFO(EAT,	     2),
	OP_INFO(GCHM,	     2),
	OP_INFO(GHLT,	     1),
	OP_INFO(GNRG,GNRG_COST),
};

struct brain *brain_new(uint16_t signature, uint16_t ram_size, uint16_t code_size)
{
	struct brain *self = malloc(sizeof(struct brain) + code_size * sizeof(struct instruction));
	self->next = NULL;
	self->refcount = 0;
	self->signature = signature;
	self->ram_size = ram_size;
	self->code_size = code_size;
	memset(self->code, -1, code_size * sizeof(struct instruction));
	return self;
}

static struct brain *copy_brain(struct brain *b)
{
	struct brain *c = malloc(sizeof(*b) + b->code_size * sizeof(*b->code));
	memcpy(c, b, sizeof(*b) + b->code_size * sizeof(*b->code));
	c->refcount = 0;
	c->next = NULL;
	return c;
}

#define MKIND_CASE_INSTR_FIELD(kind, field) \
	case MKIND_##kind: { \
		b = copy_brain(self); \
		b->code[grid_rand(g) % self->code_size].field = grid_rand(g); \
	} break

#define MKIND_CASE_INSTR_FMT(kind, field) \
	case MKIND_##kind: { \
		b = copy_brain(self); \
		uint8_t r = grid_rand(g); \
		size_t idx = grid_rand(g) % self->code_size; \
		if (b->code[idx].field == r % 4) \
			++r; \
		b->code[idx].field = r; \
	} break

struct brain *brain_mutate(struct brain *self, struct grid *g)
{
	struct brain *b;
	switch(grid_rand(g) % 6) {
	case MKIND_SIGNATURE: {
		b = copy_brain(self);
		b->signature = grid_rand(g);
	} break;
	MKIND_CASE_INSTR_FIELD(OPCODE, opcode);
	MKIND_CASE_INSTR_FMT(INSTR_LFMT, l_fmt);
	MKIND_CASE_INSTR_FMT(INSTR_RFMT, r_fmt);
	MKIND_CASE_INSTR_FIELD(INSTR_LEFT, left);
	MKIND_CASE_INSTR_FIELD(INSTR_RIGHT, right);
	}
	b->next = g->species;
	g->species = b;
	return b;
}
