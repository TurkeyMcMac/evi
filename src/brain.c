/*
 * The code for making and describing brains and their operations.
 *
 * Copyright (C) 2018 Jude Melton-Houghton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include "brain.h"

#include "grid.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define OP_INFO(name, energy, n_args) [OP_##name] = {#name, n_args, energy}

const struct opcode_info op_info[N_OPCODES] = {
	/*	Name	Energy	# of arguments*/
	OP_INFO(MOVE,	     1,	             2),
	OP_INFO(XCHG,	     2,	             2),
	OP_INFO(GFLG,	     1,	             1),
	OP_INFO(SFLG,	     1,	             1),
	OP_INFO(GIPT,	     1,	             1),
	OP_INFO(AND,	     4,	             2),
	OP_INFO(OR,	     4,	             2),
	OP_INFO(XOR,	     4,	             2),
	OP_INFO(NOT,	     3,	             1),
	OP_INFO(SHFR,	     4,	             2),
	OP_INFO(SHFL,	     4,	             2),
	OP_INFO(ADD,	     6,	             2),
	OP_INFO(SUB,	     6,	             2),
	OP_INFO(INCR,	     4,	             1),
	OP_INFO(DECR,	     4,	             1),
	OP_INFO(JUMP,	     4,	             1),
	OP_INFO(CMPR,	     6,	             2),
	OP_INFO(JMPA,	     6,	             2),
	OP_INFO(JPNA,	     6,	             2),
	OP_INFO(JMPO,	     6,	             2),
	OP_INFO(JPNO,	     6,	             2),
	OP_INFO(PICK,	     8,	             2),
	OP_INFO(DROP,	     7,	             2),
	OP_INFO(LCHM,	     7,	             2),
	OP_INFO(LNML,	     7,	             2),
	OP_INFO(BABY,	     5,	             2),
	OP_INFO(STEP,	     7,	             1),
	OP_INFO(ATTK,	     7,	             2),
	OP_INFO(CONV,	     6,	             2),
	OP_INFO(EAT,	     2,	             2),
	OP_INFO(GCHM,	     2,	             2),
	OP_INFO(GHLT,	     1,	             1),
	OP_INFO(GNRG,GNRG_COST,	             1),
};

static const struct opcode_info nop_info = {"NOP"};

struct brain *brain_new(uint16_t signature, uint16_t ram_size, uint16_t code_size)
{
	struct brain *self = malloc(offsetof(struct brain, code) + code_size * sizeof(struct instruction));
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
	struct brain *c = malloc(offsetof(struct brain, code) + b->code_size * sizeof(*b->code));
	memcpy(c, b, offsetof(struct brain, code) + b->code_size * sizeof(*b->code));
	c->refcount = 0;
	c->next = NULL;
	return c;
}

static struct brain *copy_shift_brain(const struct brain *b, uint16_t i, uint16_t n)
{
	struct brain *c = malloc(offsetof(struct brain, code) + (b->code_size + n) * sizeof(*b->code));
	memcpy(c, b, offsetof(struct brain, code) + i * sizeof(*b->code));
	memcpy(&c->code[i + n], &b->code[i], b->code_size - i);
	c->refcount = 0;
	c->next = NULL;
	return c;
}

static struct brain *copy_remove_brain(const struct brain *b, uint16_t i, uint16_t n)
{
	struct brain *c = malloc(offsetof(struct brain, code) + (b->code_size - n) * sizeof(*b->code));
	memcpy(c, b, offsetof(struct brain, code) + i * sizeof(*b->code));
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
void brain_print(const struct brain *self, FILE *dest)
{
	fprintf(dest, "signature:\t%04x\n", self->signature);
	fprintf(dest, "RAM size:\t%u\n", self->ram_size);
	fprintf(dest, "population:\t%lu\n", self->refcount);
	fprintf(dest, "code:\n");
	for (uint16_t i = 0; i < self->code_size; ++i) {
		const struct opcode_info *info;
		if (self->code[i].opcode < N_OPCODES)
			info = &op_info[self->code[i].opcode];
		else
			info = &nop_info;
		fprintf(dest, " %s\t", info->name);
		switch (info->n_args) {
		case 1:
			fprintf(dest, "%u[%04x]\n", self->code[i].l_fmt, self->code[i].left);
			break;
		case 2:
			fprintf(dest, "%u[%04x]  %u[%04x]\n",
				self->code[i].l_fmt, self->code[i].left,
				self->code[i].r_fmt, self->code[i].right);
			break;
		default:
			fprintf(dest, "\n");
			break;
		}
	}
}
