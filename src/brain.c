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

static struct brain *copy_brain(const struct brain *b)
{
	struct brain *c = malloc(sizeof(*b) + b->code_size * sizeof(*b->code));
	memcpy(c, b, sizeof(*b) + b->code_size * sizeof(*b->code));
	c->refcount = 0;
	c->next = NULL;
	return c;
}

static struct brain *copy_shift_brain(const struct brain *b, uint16_t i, uint16_t n)
{
	struct brain *c = malloc(sizeof(*b) + (b->code_size + n) * sizeof(*b->code));
	memcpy(c, b, sizeof(*b) + i * sizeof(*b->code));
	memcpy(&c->code[i + n], &b->code[i], b->code_size - i);
	c->refcount = 0;
	c->next = NULL;
	return c;
}

static struct brain *copy_remove_brain(const struct brain *b, uint16_t i, uint16_t n)
{
	struct brain *c = malloc(sizeof(*b) + (b->code_size - n) * sizeof(*b->code));
	memcpy(c, b, sizeof(*b) + i * sizeof(*b->code));
	memcpy(&c->code[i], &b->code[i + n], b->code_size - i - n);
	c->refcount = 0;
	c->next = NULL;
	return c;
}

static void random_instruction(struct instruction *instr, struct grid *g)
{
	instr->opcode = grid_rand(g);
	instr->l_fmt = grid_rand(g);
	instr->r_fmt = grid_rand(g);
	instr->left = grid_rand(g);
	instr->right = grid_rand(g);
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

enum {
	MKIND_RAM_SIZE,
	MKIND_SIGNATURE,
	MKIND_OPCODE,
	MKIND_INSTR_LFMT,
	MKIND_INSTR_RFMT,
	MKIND_INSTR_LEFT,
	MKIND_INSTR_RIGHT,
	MKIND_ADD,
	MKIND_REPLACE,
	MKIND_REMOVE,
	MKIND_DUPLICATE,
	MKIND_ROTATE,

	N_MKIND
};

#define MAX_RAM 1024
#define MAX_ROT_SIZE 32

struct brain *brain_mutate(const struct brain *self, struct grid *g)
{
	struct brain *b;
	switch(grid_rand(g) % N_MKIND) {
	case MKIND_RAM_SIZE: {
		b = copy_brain(self);
		b->ram_size = (b->ram_size + (grid_rand(g) & 1) * 2 - 1) % MAX_RAM;
	} break;
	case MKIND_SIGNATURE: {
		b = copy_brain(self);
		b->signature = grid_rand(g);
	} break;
	MKIND_CASE_INSTR_FIELD(OPCODE, opcode);
	MKIND_CASE_INSTR_FMT(INSTR_LFMT, l_fmt);
	MKIND_CASE_INSTR_FMT(INSTR_RFMT, r_fmt);
	MKIND_CASE_INSTR_FIELD(INSTR_LEFT, left);
	MKIND_CASE_INSTR_FIELD(INSTR_RIGHT, right);
	case MKIND_ADD: {
		uint16_t idx = grid_rand(g) % self->code_size;
		b = copy_shift_brain(self, idx, 1);
		random_instruction(&b->code[idx], g);
		++b->code_size;
	} break;
	case MKIND_REPLACE: {
		b = copy_brain(self);
		random_instruction(&b->code[grid_rand(g) % self->code_size], g);
	} break;
	case MKIND_REMOVE: {
		if (self->code_size == 1) {
			b = copy_brain(self);
			break;
		}
		b = copy_remove_brain(self, grid_rand(g) % self->code_size, 1);
		--b->code_size;
	} break;
	case MKIND_DUPLICATE: {
		uint16_t idx = grid_rand(g) % self->code_size;
		b = copy_shift_brain(self, idx, 1);
		b->code[idx + 1] = b->code[idx];
		++b->code_size;
	} break;
	case MKIND_ROTATE: {
		uint16_t size1 = grid_rand(g) % MAX_ROT_SIZE % self->code_size,
			 size2 = grid_rand(g) % MAX_ROT_SIZE % (self->code_size - size1);
		uint16_t i;
		if (size1 + size2 == self->code_size)
			i = 0;
		else
			i = grid_rand(g) % (self->code_size - size1 - size2);
		b = copy_brain(self);
		memcpy(&b->code[i], &self->code[i + size1], size2 * sizeof(*self->code));
		memcpy(&b->code[i + size2], &self->code[i], size1 * sizeof(*self->code));
	} break;
	}
	b->next = g->species;
	g->species = b;
	return b;
}
