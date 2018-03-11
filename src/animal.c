#include "animal.h"

#define OPCODE_MASK ((1 << 12) - 1)

#define left_arg_format(op) ((op) >> 14)
#define right_arg_format(op) (((op) >> 12) & 3)

#include <stdio.h>

enum {
	ARG_FMT_IMMEDIATE = 0,
	ARG_FMT_FOLLOW_ONCE = 1,
	ARG_FMT_FOLLOW_TWICE = 2,
};

void set_error(struct animal *a, uint16_t errs)
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
			set_error(a, FROOB);
			return -1;
		}
		break;
	case ARG_FMT_FOLLOW_TWICE:
		if (value < a->brain->ram_size && a->ram[value] < a->brain->ram_size)
			*dest = a->ram[a->ram[value]];
		else {
			set_error(a, FROOB);
			return -1;
		}
		break;
	default:
		set_error(a, FINVAL_ARG);
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
			set_error(a, FROOB);
			return NULL;
		}
	case ARG_FMT_FOLLOW_TWICE:
		if (value < a->brain->ram_size && a->ram[value] < a->brain->ram_size) {
			return &a->ram[a->ram[value]];
		} else {
			set_error(a, FROOB);
			return NULL;
		}
	default:
		set_error(a, FINVAL_ARG);
		return NULL;
	}
}

#define get_arg(self, offset) ((self)->brain->code[(self)->instr_ptr + (offset)])

static const uint16_t arg_nums[] = {
/* General */
	[OP_MOVE] = 2,
	[OP_XCHG] = 2,
	[OP_FLAG] = 1,
	[OP_LFLG] = 1,
/* Bitwise */
	[OP_AND ] = 2,
	[OP_OR  ] = 2,
	[OP_XOR ] = 2,
	[OP_NOT ] = 1,
	[OP_SHFR] = 2,
	[OP_SHFL] = 2,
/* Arithmetic */
	[OP_ADD ] = 2,
	[OP_SUB ] = 2,
	[OP_MUL ] = 2,
	[OP_SMUL] = 2,
	[OP_DIV ] = 2,
	[OP_SDIV] = 2,
	[OP_MOD ] = 2,
	[OP_SMOD] = 2,
/* Control flow */
	[OP_JUMP] = 1,
	[OP_CMPR] = 2,
	[OP_JPTA] = 2,
	[OP_JTNA] = 2,
	[OP_JPTO] = 2,
	[OP_JTNO] = 2,
/* Special */
	[OP_FACE] = 1,
	[OP_PICK] = 2,
	[OP_DROP] = 2,
	[OP_CONV] = 2,
	[OP_EAT ] = 1,
	[OP_CHEM] = 2,
	[OP_BABY] = 0,
};

static int jump(struct animal *a, uint16_t dest, uint16_t arg_num)
{
	if (dest >= a->brain->code_size) {
		set_error(a, FCOOB);
		a->instr_ptr += 1 + arg_num;
		return -1;
	} else {
		a->instr_ptr = dest;
		return 0;
	}
}

#define _OP_CASE(name, type, action) \
	case OP_##name: { \
		type temp, *dest = (type *)write_dest(self, l_fmt, get_arg(self, 1)); \
		if (!dest || read_from(self, r_fmt, get_arg(self, 2), (type *)&temp)) \
			break; \
		*dest action##= temp; \
	} break

#define OP_CASE_UNSIGNED(name, action) _OP_CASE(name, uint16_t, action)
#define OP_CASE_SIGNED(name, action) _OP_CASE(S##name, int16_t, action)

#define OP_CASE_JUMP_COND(name, condition) \
	case OP_##name: { \
		uint16_t dest, test; \
		if (read_from(self, l_fmt, get_arg(self, 1), &dest) \
		 || read_from(self, r_fmt, get_arg(self, 2), &test)) \
			goto error; \
		if (condition) { \
			if (jump(self, dest, arg_nums[opcode])) \
				goto error; \
			else \
				return 0; \
		} \
	} break

int animal_step(struct animal *self)
{
	uint16_t op = self->brain->code[self->instr_ptr];
	uint16_t l_fmt = left_arg_format(op),
		 r_fmt = right_arg_format(op);
	uint16_t opcode = op & OPCODE_MASK;
	if (opcode >= N_OPCODES) {
		set_error(self, FINVAL_OPCODE);
		++self->instr_ptr;
		return -1;
	}
	if (self->instr_ptr + arg_nums[opcode] >= self->brain->code_size) {
		set_error(self, FINVAL_OPCODE);
		++self->instr_ptr;
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
	case OP_FLAG: {
		uint16_t *dest = write_dest(self, l_fmt, get_arg(self, 1));
		if (dest)
			*dest = self->flags;
		else
			goto error;
	} break;
	case OP_LFLG: {
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
	OP_CASE_UNSIGNED(MUL, *);
	OP_CASE_SIGNED(MUL, *);
	OP_CASE_UNSIGNED(DIV, /);
	OP_CASE_SIGNED(DIV, /);
	OP_CASE_UNSIGNED(MOD, %);
	OP_CASE_SIGNED(MOD, %);
/* Control flow */
	case OP_JUMP: {
		uint16_t dest;
		if (read_from(self, l_fmt, get_arg(self, 1), &dest)
		 || jump(self, dest, arg_nums[opcode]))
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
	OP_CASE_JUMP_COND(JPTA, self->flags & test == test);
	OP_CASE_JUMP_COND(JTNA, self->flags & test == 0);
	OP_CASE_JUMP_COND(JPTO, (self->flags & test) != 0);
	OP_CASE_JUMP_COND(JTNO, self->flags & test != test);
/* Special */
	default: {
		set_error(self, FINVAL_OPCODE);
	} break;
	}
	self->flags &= ~(FINVAL_ARG | FROOB | FCOOB | FINVAL_OPCODE);
error:
	self->instr_ptr += 1 + arg_nums[opcode];
	return 0;
}

#define INSTR(i, lf, rf) (OP_##i | ((lf) << 14) | ((rf) << 12))

#define I ARG_FMT_IMMEDIATE
#define F ARG_FMT_FOLLOW_ONCE
#define E ARG_FMT_FOLLOW_TWICE

static struct brain test_brain = {
	.ram_size = 6,
	.code_size = 9,
	.code = {
		INSTR(SUB, F,I),	0x0000, 1,
		INSTR(CMPR, F,I),	0x0000, 0,
		INSTR(JPTO, I,I),	0x0000, FGREATER,
	},
};

static struct animal test_animal = {
	.instr_ptr = 0,
	.brain = &test_brain,
	.flags = 0,
	.ram = {6000,0,0,0,0}
};

void test_asm(void)
{
	int i = 0;
	while (test_animal.instr_ptr < test_brain.code_size) {
		printf("countdown: %u\n", test_animal.ram[0]);
		animal_step(&test_animal);
		++i;
	}
	printf("%d\n", i);
}
