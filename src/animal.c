#include "animal.h"

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

static int read_from(struct animal *a, uint_fast8_t fmt, uint16_t value, uint16_t *dest)
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

static uint16_t *write_dest(struct animal *a, uint_fast8_t fmt, uint16_t value)
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

#define OP_CASE_NUMERIC_BINARY(name, action) \
	case OP_##name: { \
		uint16_t temp, *dest = write_dest(self, instr->l_fmt, instr->left); \
		if (!dest || read_from(self, instr->r_fmt, instr->right, &temp)) \
			break; \
		*dest action##= temp; \
	} break

#define OP_CASE_NUMERIC_UNARY(name, action) \
	case OP_##name: { \
		uint16_t *dest = write_dest(self, instr->l_fmt, instr->left); \
		if (dest) \
			(action); \
		else \
			goto error; \
	} break;

#define OP_CASE_JUMP_COND(name, condition) \
	case OP_##name: { \
		uint16_t dest, test; \
		if (read_from(self, instr->l_fmt, instr->left, &dest) \
		 || read_from(self, instr->r_fmt, instr->right, &test)) \
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
	struct instruction *instr = &self->brain->code[self->instr_ptr];
	if (instr->opcode >= N_OPCODES) {
		set_instr_error(self, FINVAL_OPCODE);
		++self->instr_ptr;
		return 0;
	}
	switch (instr->opcode) {
/* General */
	case OP_MOVE: {
		uint16_t *dest = write_dest(self, instr->l_fmt, instr->left);
		if (!dest || read_from(self, instr->r_fmt, instr->right, dest))
			goto error;
	} break;
	case OP_XCHG: {
		uint16_t temp, *destl, *destr;
		if ((destl = write_dest(self, instr->l_fmt, instr->left)) == NULL
		 || (destr = write_dest(self, instr->r_fmt, instr->right)) == NULL)
			goto error;
		temp = *destl;
		*destl = *destr;
		*destr = temp;
	} break;
	case OP_GFLG: {
		uint16_t *dest = write_dest(self, instr->l_fmt, instr->left);
		if (dest)
			*dest = self->flags;
		else
			goto error;
	} break;
	case OP_SFLG: {
		if (read_from(self, instr->l_fmt, instr->left, &self->flags))
			goto error;
	} break;
/* Bitwise */
	OP_CASE_NUMERIC_BINARY(AND, &);
	OP_CASE_NUMERIC_BINARY(OR, |);
	OP_CASE_NUMERIC_BINARY(XOR, ^);
	OP_CASE_NUMERIC_UNARY(NOT, *dest = ~*dest);
	OP_CASE_NUMERIC_BINARY(SHFR, >>);
	OP_CASE_NUMERIC_BINARY(SHFL, <<);
/* Arithmetic */
	OP_CASE_NUMERIC_BINARY(ADD, +);
	OP_CASE_NUMERIC_BINARY(SUB, -);
	OP_CASE_NUMERIC_UNARY(INCR, ++*dest);
	OP_CASE_NUMERIC_UNARY(DECR, --*dest);
/* Control flow */
	case OP_JUMP: {
		uint16_t dest;
		if (read_from(self, instr->l_fmt, instr->left, &dest)
		 || jump(self, dest))
			goto error;
	} return 0;
	case OP_CMPR: {
		uint16_t left, right;
		if (read_from(self, instr->l_fmt, instr->left, &left)
		 || read_from(self, instr->r_fmt, instr->right, &right))
			goto error;
		if (left > right) {
			self->flags |= FUGREATER;
			self->flags &= ~(FULESSER | FEQUAL);
		} else if (left < right) {
			self->flags |= FULESSER;
			self->flags &= ~(FEQUAL | FUGREATER);
		} else {
			self->flags |= FEQUAL;
			self->flags &= ~(FULESSER | FUGREATER | FSLESSER | FSGREATER);
			break;
		}
		if ((int16_t)left > (int16_t)right) {
			self->flags |= FSGREATER;
			self->flags &= ~FSLESSER;
		} else if ((int16_t)left < (int16_t)right) {
			self->flags |= FSLESSER;
			self->flags &= ~FSGREATER;
		}
	} break;
	OP_CASE_JUMP_COND(JMPA, (self->flags | test) == self->flags);
	OP_CASE_JUMP_COND(JPNA, (self->flags & test) == 0);
	OP_CASE_JUMP_COND(JMPO, (self->flags & test) != 0);
	OP_CASE_JUMP_COND(JPNO, (self->flags & test) != test);
/* Special */
	default: {
		set_instr_error(self, FINVAL_OPCODE);
	} goto error;
	}
	self->flags &= ~(FINVAL_ARG | FROOB | FCOOB | FINVAL_OPCODE);
error:
	++self->instr_ptr;
	return 0;
}

#define INSTR2(opcode, l_fmt, left, r_fmt, right) {OP_##opcode, l_fmt, r_fmt, left, right}
#define INSTR1(_opcode, arg_fmt, arg) {.opcode = OP_##_opcode, .l_fmt = arg_fmt, .left = arg}
#define INSTR0(opcode) {OP_##opcode}

#define IM ARG_FMT_IMMEDIATE
#define F1 ARG_FMT_FOLLOW_ONCE
#define F2 ARG_FMT_FOLLOW_TWICE

static struct brain test_brain = {
	.ram_size = 6,
	.code_size = 3,
	.code = {
		[0x0000] = INSTR1(INCR, F1,0x0000),
		[0x0001] = INSTR2(CMPR, F1,0x0000, IM,0),
		[0x0002] = INSTR2(JMPO, IM,0x0000, IM,FSLESSER),
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
