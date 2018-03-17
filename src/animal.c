#include "animal.h"

#include <limits.h>

#define OP_INFO(name, chem, energy) [OP_##name] = {#name, CHEM_M##chem, energy}

#define GNRG_COST 1

const struct opcode_info op_info[N_OPCODES] = {
	/*	Name	Chem	Energy */
	OP_INFO(MOVE,	GENRL,	     1),
	OP_INFO(XCHG,	GENRL,	     2),
	OP_INFO(GFLG,	GENRL,	     1),
	OP_INFO(SFLG,	GENRL,	     1),
	OP_INFO(GIPT,	GENRL,	     1),
	OP_INFO(AND,	BIT,	     4),
	OP_INFO(OR,	BIT,	     4),
	OP_INFO(XOR,	BIT,	     4),
	OP_INFO(NOT,	BIT,	     3),
	OP_INFO(SHFR,	BIT,	     4),
	OP_INFO(SHFL,	BIT,	     4),
	OP_INFO(ADD,	ARITH,	     6),
	OP_INFO(SUB,	ARITH,	     6),
	OP_INFO(INCR,	ARITH,	     4),
	OP_INFO(DECR,	ARITH,	     4),
	OP_INFO(JUMP,	CNTRL,	     4),
	OP_INFO(CMPR,	CNTRL,	     6),
	OP_INFO(JMPA,	CNTRL,	     6),
	OP_INFO(JPNA,	CNTRL,	     6),
	OP_INFO(JMPO,	CNTRL,	     6),
	OP_INFO(JPNO,	CNTRL,	     6),
	OP_INFO(PICK,	SPEC,	     8),
	OP_INFO(DROP,	SPEC,	     7),
	OP_INFO(LCHM,	SPEC,	     3),
	OP_INFO(LNML,	SPEC,	     3),
	OP_INFO(BABY,	SPEC,	     5),
	OP_INFO(CONV,	SPEC,	    10),
	OP_INFO(EAT,	SPEC,	     2),
	OP_INFO(GCHM,	SPEC,	     2),
	OP_INFO(GHLT,	SPEC,	     1),
	OP_INFO(GNRG,	SPEC,	GNRG_COST),
};

#define bits_on(bitset, bits) ((bitset) |= (bits))
#define bits_off(bitset, bits) ((bitset) &= ~(bits))
#define FERRORS (FINVAL_ARG | FROOB | FCOOB | FINVAL_OPCODE | FNO_RESOURCE)

enum {
	ARG_FMT_IMMEDIATE = 0,
	ARG_FMT_FOLLOW_ONCE = 1,
	ARG_FMT_FOLLOW_TWICE = 2,
};

#define paste2(t1, t2) t1##t2
#define paste1(t1, t2) paste2(t1, t2)

#define set_error(a, errs) do { \
	struct animal *paste2(_##a##_line_, __LINE__)  = (a); \
	bits_off(paste2(_##a##_line_, __LINE__)->flags, FERRORS); \
	bits_on(paste2(_##a##_line_, __LINE__)->flags, (errs)); \
} while (0)

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

static uint16_t *write_dest(struct animal *a, uint_fast8_t fmt, uint16_t value)
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

static int jump(struct animal *a, uint16_t dest)
{
	if (dest >= a->brain->code_size) {
		set_error(a, FCOOB);
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
				return; \
		} \
	} break

void animal_step(struct animal *self)
{
	if (self->instr_ptr >= self->brain->code_size)
		return;
	struct instruction *instr = &self->brain->code[self->instr_ptr];
	if (instr->opcode >= N_OPCODES) {
		set_error(self, FINVAL_OPCODE);
		goto error;
	}
	self->action.opcode = 0;
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
	case OP_GIPT: {
		uint16_t *dest = write_dest(self, instr->l_fmt, instr->left);
		if (dest)
			*dest = self->instr_ptr;
		else
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
	} return;
	case OP_CMPR: {
		uint16_t left, right;
		if (read_from(self, instr->l_fmt, instr->left, &left)
		 || read_from(self, instr->r_fmt, instr->right, &right))
			goto error;
		if (left > right) {
			bits_on(self->flags, FUGREATER);
			bits_off(self->flags, FULESSER | FEQUAL);
		} else if (left < right) {
			bits_on(self->flags, FULESSER);
			bits_off(self->flags, FUGREATER | FEQUAL);
		} else {
			bits_on(self->flags, FEQUAL);
			bits_off(self->flags, FULESSER | FUGREATER | FSLESSER | FSGREATER);
			break;
		}
		if ((int16_t)left > (int16_t)right) {
			bits_on(self->flags, FSGREATER);
			bits_off(self->flags, FSLESSER);
		} else if ((int16_t)left < (int16_t)right) {
			bits_on(self->flags, FSLESSER);
			bits_off(self->flags, FSGREATER);
		}
	} break;
	OP_CASE_JUMP_COND(JMPA, (self->flags | test) == self->flags);
	OP_CASE_JUMP_COND(JPNA, (self->flags & test) == 0);
	OP_CASE_JUMP_COND(JMPO, (self->flags & test) != 0);
	OP_CASE_JUMP_COND(JPNO, (self->flags & test) != test);
/* Special */
	case OP_PICK: case OP_DROP: case OP_LCHM: case OP_LNML: case OP_BABY: {
		/* This is an interaction with the world, so it must be handled externally. */
		self->action = *instr;
	} break;
	case OP_CONV: {
		uint16_t c1, c2;
		if (read_from(self, instr->l_fmt, instr->left, &c1)
		 || read_from(self, instr->r_fmt, instr->right, &c2))
			goto error;
		if (c1 >= N_CHEMICALS || c2 >= N_CHEMICALS) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		if (self->stomach[c1] > 0 && self->stomach[c2] > 0) {
			--self->stomach[c1];
			--self->stomach[c2];
			++self->stomach[combine_chemicals(c1, c2)];
		} else {
			set_error(self, FNO_RESOURCE);
			goto error;
		}
	} break;
	case OP_EAT: {
		uint16_t chem, amount;
		if (read_from(self, instr->l_fmt, instr->left, &chem)
		 || read_from(self, instr->r_fmt, instr->right, &amount))
			goto error;
		if (chem >= N_CHEMICALS) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		amount &= UINT8_MAX;
		if (amount > self->stomach[chem]) {
			set_error(self, FNO_RESOURCE);
			goto error;
		} else {
			self->stomach[chem] -= amount;
			self->energy += amount * chemical_table[chem].energy;
		}
	} break;
	case OP_GCHM: {
		uint16_t chem, *dest = write_dest(self, instr->l_fmt, instr->left);
		if (!dest || read_from(self, instr->r_fmt, instr->right, &chem))
			goto error;
		if (chem >= N_CHEMICALS) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		*dest = self->stomach[chem];
	} break;
	case OP_GHLT: {
		uint16_t *dest = write_dest(self, instr->l_fmt, instr->left);
		if (dest)
			*dest = self->health;
		else
			goto error;
	} break;
	case OP_GNRG: {
		uint16_t *dest = write_dest(self, instr->l_fmt, instr->left);
		if (dest)
			*dest = self->energy - GNRG_COST; /* We don't have to deal with underflow
			                                     because if it occurs, the animal will
							     die immediately before being able to
							     react. */
		else
			goto error;
	} break;
	default: {
		set_error(self, FINVAL_OPCODE);
	} goto error;
	}
	bits_off(self->flags, FERRORS);
	if (self->energy < op_info[instr->opcode].energy)
		self->energy = 0;
	else
		self->energy -= op_info[instr->opcode].energy;
error:
	++self->instr_ptr;
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
		[0x0000] = INSTR1(GNRG, F1,0x0000),
		[0x0001] = INSTR2(CMPR, F1,0x0000, IM,100),
		[0x0002] = INSTR2(JMPO, IM,0x0000, IM,FUGREATER),
	},
};

static struct animal test_animal = {
	.instr_ptr = 0,
	.brain = &test_brain,
	.flags = 0,
	.energy = 1000,
	.ram = {0,0,0,0,0}
};

#include <stdio.h>

void test_asm(void)
{
	printf("%s\n", op_info[OP_BABY].name);
	do {
		animal_step(&test_animal);
		printf("countdown: %u\n", test_animal.ram[0]);
		printf("flags: %u\n", test_animal.flags);
	} while (test_animal.instr_ptr < test_animal.brain->code_size && test_animal.energy > 0);
	printf("energy: %u\n", test_animal.energy);
}
