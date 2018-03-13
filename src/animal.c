#include "animal.h"

#define OPCODE_MASK ((1 << 10) - 1)

#define left_arg_format(op) (((op) >> 12) & 3)
#define right_arg_format(op) (((op) >> 10) & 3)
#define operation_size(op) ((op) >> 14)

enum {
	ARG_FMT_IMMEDIATE = 0,
	ARG_FMT_FOLLOW_ONCE = 1,
	ARG_FMT_FOLLOW_TWICE = 2,
};

void set_instr_error(struct animal *a, uint16_t errs)
{
	a->flags |= errs;
	a->flags &= errs ^ ~(FINVAL_ARG | FROOB | FCOOB | FINVAL_OPCODE);
}

static int read_from(struct animal *a, uint16_t fmt, uint16_t value, uint16_t *dest)
{
	switch (fmt) {
	case ARG_FMT_IMMEDIATE:
		*dest = value;
		break;
	case ARG_FMT_FOLLOW_ONCE:
		if (value < a->brain->ram_size)
			*dest = a->ram[value];
		else {
			set_instr_error(a, FROOB);
			return -1;
		}
		break;
	case ARG_FMT_FOLLOW_TWICE:
		if (value < a->brain->ram_size && a->ram[value] < a->brain->ram_size)
			*dest = a->ram[a->ram[value]];
		else {
			set_instr_error(a, FROOB);
			return -1;
		}
		break;
	default:
		set_instr_error(a, FINVAL_ARG);
		return -1;
	}
	return 0;
}

static uint16_t *write_dest(struct animal *a, uint16_t fmt, uint16_t value)
{
	switch (fmt) {
	case ARG_FMT_FOLLOW_ONCE:
		if (value < a->brain->ram_size) {
			return &a->ram[value];
		} else {
			set_instr_error(a, FROOB);
			return NULL;
		}
	case ARG_FMT_FOLLOW_TWICE:
		if (value < a->brain->ram_size && a->ram[value] < a->brain->ram_size) {
			return &a->ram[a->ram[value]];
		} else {
			set_instr_error(a, FROOB);
			return NULL;
		}
	default:
		set_instr_error(a, FINVAL_ARG);
		return NULL;
	}
}

#define get_arg(self, offset) ((self)->brain->code[(self)->instr_ptr + (offset)])

static int jump(struct animal *a, uint16_t dest)
{
	if (dest >= a->brain->code_size) {
		set_instr_error(a, FCOOB);
		return -1;
	} else {
		a->instr_ptr = dest;
		return 0;
	}
}

#define OP_CASE_NUMERIC(name, type, action) \
	case OP_##name: { \
		type temp, *dest = (type *)write_dest(self, l_fmt, get_arg(self, 1)); \
		if (!dest || read_from(self, r_fmt, get_arg(self, 2), (type *)&temp)) \
			break; \
		*dest action##= temp; \
	} break

#define OP_CASE_UNSIGNED(name, action) OP_CASE_NUMERIC(name, uint16_t, action)

#define OP_CASE_JUMP_COND(name, condition) \
	case OP_##name: { \
		uint16_t dest, test; \
		if (read_from(self, l_fmt, get_arg(self, 1), &dest) \
		 || read_from(self, r_fmt, get_arg(self, 2), &test)) \
			goto error; \
		if (condition) { \
			if (jump(self, dest)) \
				goto error; \
			else \
				return 0; \
		} \
	} break

int animal_step(struct animal *self)
{
	if (self->instr_ptr >= self->brain->code_size)
		return -1;
	uint16_t op = self->brain->code[self->instr_ptr];
	uint16_t op_size = operation_size(op),
		 l_fmt = left_arg_format(op),
		 r_fmt = right_arg_format(op);
	uint16_t opcode = op & OPCODE_MASK;
	if (opcode >= N_OPCODES) {
		set_instr_error(self, FINVAL_OPCODE);
		++self->instr_ptr;
		return 0;
	}
	if (self->instr_ptr + op_size > self->brain->code_size) {
		return -1;
	}
	switch (opcode) {
/* General */
	case OP_MOVE: {
		uint16_t *dest = write_dest(self, l_fmt, get_arg(self, 1));
		if (!dest || read_from(self, r_fmt, get_arg(self, 2), dest))
			goto error;
	} break;
	case OP_XCHG: {
		uint16_t temp, *destl, *destr;
		if ((destl = write_dest(self, l_fmt, get_arg(self, 1))) == NULL
		 || (destr = write_dest(self, r_fmt, get_arg(self, 2))) == NULL)
			goto error;
		temp = *destl;
		*destl = *destr;
		*destr = temp;
	} break;
	case OP_GFLG: {
		uint16_t *dest = write_dest(self, l_fmt, get_arg(self, 1));
		if (dest)
			*dest = self->flags;
		else
			goto error;
	} break;
	case OP_SFLG: {
		if (read_from(self, r_fmt, get_arg(self, 1), &self->flags))
			goto error;
	} break;
/* Bitwise */
	OP_CASE_UNSIGNED(AND, &);
	OP_CASE_UNSIGNED(OR, |);
	OP_CASE_UNSIGNED(XOR, ^);
	case OP_NOT: {
		uint16_t *dest = write_dest(self, l_fmt, get_arg(self, 1));
		if (dest)
			*dest = ~*dest;
	} break;
	OP_CASE_UNSIGNED(SHFR, >>);
	OP_CASE_UNSIGNED(SHFL, <<);
/* Arithmetic */
	OP_CASE_UNSIGNED(ADD, +);
	OP_CASE_UNSIGNED(SUB, -);
/* Control flow */
	case OP_JUMP: {
		uint16_t dest;
		if (read_from(self, l_fmt, get_arg(self, 1), &dest)
		 || jump(self, dest))
			goto error;
	} return 0;
	case OP_CMPR: {
		int16_t left, right;
		if (read_from(self, l_fmt, get_arg(self, 1), (uint16_t *)&left)
		 || read_from(self, r_fmt, get_arg(self, 2), (uint16_t *)&right))
			goto error;
		if (left > right) {
			self->flags |= FGREATER;
			self->flags &= ~(FLESSER | FEQUAL);
		} else if (left < right) {
			self->flags |= FLESSER;
			self->flags &= ~(FEQUAL | FGREATER);
		} else {
			self->flags |= FEQUAL;
			self->flags &= ~(FLESSER | FGREATER);
		}
	} break;
	OP_CASE_JUMP_COND(JPTA, (self->flags | test) == self->flags);
	OP_CASE_JUMP_COND(JTNA, (self->flags & test) == 0);
	OP_CASE_JUMP_COND(JPTO, (self->flags & test) != 0);
	OP_CASE_JUMP_COND(JTNO, (self->flags & test) != test);
/* Special */
	default: {
		set_instr_error(self, FINVAL_OPCODE);
	} goto error;
	}
	self->flags &= ~(FINVAL_ARG | FROOB | FCOOB | FINVAL_OPCODE);
error:
	self->instr_ptr += op_size;
	return 0;
}

/*                                   left format    right format   op size */
#define INSTR2(i, lf, rf) (OP_##i | ((lf) << 12) | ((rf) << 10) | (3 << 14))
#define INSTR1(i, lf, rf) (OP_##i | ((lf) << 12) | ((rf) << 10) | (2 << 14))
#define INSTR0(i, lf, rf) (OP_##i | ((lf) << 12) | ((rf) << 10) | (1 << 14))

#define I ARG_FMT_IMMEDIATE
#define F ARG_FMT_FOLLOW_ONCE
#define E ARG_FMT_FOLLOW_TWICE

static struct brain test_brain = {
	.ram_size = 6,
	.code_size = 9,
	.code = {
		[0x0000] = INSTR2(ADD,  F,I),	0x0000, 1,
		[0x0003] = INSTR2(CMPR, F,I),	0x0000, 1,
		[0x0006] = INSTR2(JPTO, I,I),	0x0000, FLESSER,
	},
};

static struct animal test_animal = {
	.instr_ptr = 0,
	.brain = &test_brain,
	.flags = 0,
	.ram = {-6000,0,0,0,0}
};

#include <stdio.h>

void test_asm(void)
{
	do {
		printf("countdown: %u\n", test_animal.ram[0]);
		printf("flags: %u\n", test_animal.flags);
	} while (!animal_step(&test_animal));
}
