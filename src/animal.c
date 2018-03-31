#include "animal.h"

#include "brain.h"
#include "grid.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define bits_on(bitset, bits) ((bitset) |= (bits))
#define bits_off(bitset, bits) ((bitset) &= ~(bits))
#define FERRORS (FINVAL_ARG | FROOB | FCOOB | FINVAL_OPCODE | FEMPTY | FFULL | FBLOCKED)

static void sub_saturate(uint16_t *dest, uint16_t src)
{
	if (*dest < src)
		*dest = 0;
	else
		*dest -= src;
}

static void add_saturate(uint16_t *dest, uint16_t src)
{
	if ((long)*dest + (long)src > UINT16_MAX)
		*dest = UINT16_MAX;
	else
		*dest += src;
}

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
		uint16_t temp, *dest = write_dest(self, instr.l_fmt, instr.left); \
		if (!dest || read_from(self, instr.r_fmt, instr.right, &temp)) \
			break; \
		*dest action##= temp; \
	} break

#define OP_CASE_NUMERIC_UNARY(name, action) \
	case OP_##name: { \
		uint16_t *dest = write_dest(self, instr.l_fmt, instr.left); \
		if (dest) \
			(action); \
		else \
			goto error; \
	} break;

#define OP_CASE_JUMP_COND(name, condition) \
	case OP_##name: { \
		uint16_t dest, test; \
		if (read_from(self, instr.l_fmt, instr.left, &dest) \
		 || read_from(self, instr.r_fmt, instr.right, &test)) \
			goto error; \
		if (condition) { \
			if (jump(self, dest)) \
				goto error; \
			else \
				return; \
		} \
	} break


enum {
	DIRECTION_UP,
	DIRECTION_RIGHT,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_HERE,
};

static struct tile *in_direction(struct grid *g, uint16_t direction, size_t x, size_t y)
{
	switch (direction) {
	case DIRECTION_UP:
		--y;
		break;
	case DIRECTION_RIGHT:
		++x;
		break;
	case DIRECTION_DOWN:
		++y;
		break;
	case DIRECTION_LEFT:
		--x;
		break;
	case DIRECTION_HERE:
		break;
	default:
		return (void *)(ptrdiff_t)-1;
	}
	return grid_get(g, x, y);
}

static void transfer(struct animal *a,
		uint8_t dest[N_CHEMICALS],
		uint8_t src[N_CHEMICALS],
		uint8_t num,
		uint8_t id)
{
	if (id >= N_CHEMICALS) {
		set_error(a, FINVAL_ARG);
		return;
	}
	bits_off(a->flags, FERRORS);
	if (src[id] < num) {
		bits_on(a->flags, FEMPTY);
		num = src[id];
	}
	if ((unsigned)dest[id] + (unsigned)num > UINT8_MAX) {
		bits_on(a->flags, FFULL);
		num = UINT8_MAX - dest[id];
	}
	dest[id] += num;
	src[id] -= num;
}

const struct tile *get_relative(const struct grid *g,
		uint16_t relative_x_and_y,
		size_t x,
		size_t y)
{
	size_t relative_x = (relative_x_and_y >> 5) & ((1 << 4) - 1),
	       relative_y = relative_x_and_y & ((1 << 4) - 1);
	if (relative_x_and_y & (1 << 9)) {
		relative_x |= ~0xF;
	}
	if (relative_x_and_y & (1 << 5)) {
		relative_y |= ~0xF;
	}
	x += relative_x;
	y += relative_y;
	return grid_get_const(g, x, y);
}
void animal_step(struct animal *self, struct grid *g, size_t x, size_t y)
{
	if (self->instr_ptr >= self->brain->code_size)
		return;
	struct instruction instr = self->brain->code[self->instr_ptr];
	if (instr.opcode >= N_OPCODES) {
		set_error(self, FINVAL_OPCODE);
		goto error;
	}
	switch (instr.opcode) {
/* General */
	case OP_MOVE: {
		uint16_t *dest = write_dest(self, instr.l_fmt, instr.left);
		if (!dest || read_from(self, instr.r_fmt, instr.right, dest))
			goto error;
	} break;
	case OP_XCHG: {
		uint16_t temp, *destl, *destr;
		if ((destl = write_dest(self, instr.l_fmt, instr.left)) == NULL
		 || (destr = write_dest(self, instr.r_fmt, instr.right)) == NULL)
			goto error;
		temp = *destl;
		*destl = *destr;
		*destr = temp;
	} break;
	case OP_GFLG: {
		uint16_t *dest = write_dest(self, instr.l_fmt, instr.left);
		if (dest)
			*dest = self->flags;
		else
			goto error;
	} break;
	case OP_SFLG: {
		if (read_from(self, instr.l_fmt, instr.left, &self->flags))
			goto error;
	} break;
	case OP_GIPT: {
		uint16_t *dest = write_dest(self, instr.l_fmt, instr.left);
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
		if (read_from(self, instr.l_fmt, instr.left, &dest)
		 || jump(self, dest))
			goto error;
	} return;
	case OP_CMPR: {
		uint16_t left, right;
		if (read_from(self, instr.l_fmt, instr.left, &left)
		 || read_from(self, instr.r_fmt, instr.right, &right))
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
	case OP_PICK: {
		uint16_t direction, num_and_id;
		if (read_from(self, instr.l_fmt, instr.left, &direction)
		 || read_from(self, instr.r_fmt, instr.right, &num_and_id))
			goto error;
		struct tile *targ = in_direction(g, direction, x, y);
		if ((ptrdiff_t)targ == -1) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		if (!targ) {
			set_error(self, FBLOCKED);
			goto error;
		}
		uint8_t num = num_and_id >> 8,
			id = num_and_id & UINT8_MAX;
		transfer(self, self->stomach, targ->chemicals, num, id);
	} break;
	case OP_DROP: {
		uint16_t direction, num_and_id;
		if (read_from(self, instr.l_fmt, instr.left, &direction)
		 || read_from(self, instr.r_fmt, instr.right, &num_and_id))
			goto error;
		struct tile *targ = in_direction(g, direction, x, y);
		if ((ptrdiff_t)targ == -1) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		if (!targ) {
			set_error(self, FBLOCKED);
			goto error;
		}
		uint8_t num = num_and_id >> 8,
			id = num_and_id & UINT8_MAX;
		transfer(self, targ->chemicals, self->stomach, num, id);
	} break;
	case OP_LCHM: {
		uint16_t id_and_x_and_y, *dest =
			write_dest(self, instr.l_fmt, instr.left);
		if (!dest
		 || read_from(self, instr.r_fmt, instr.right, &id_and_x_and_y))
			goto error;
		const struct tile *look = get_relative(g, id_and_x_and_y, x, y);
		if (!look) {
			set_error(self, FBLOCKED);
			goto error;
		}
		uint16_t id = id_and_x_and_y >> 10;
		if (id >= N_CHEMICALS) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		*dest = look->chemicals[id];
	} break;
	case OP_LNML: {
		uint16_t x_and_y, *dest = write_dest(self, instr.l_fmt, instr.left);
		if (!dest || read_from(self, instr.r_fmt, instr.right, &x_and_y))
			goto error;
		const struct tile *look = get_relative(g, x_and_y, x, y);
		if (!look) {
			set_error(self, FBLOCKED);
			goto error;
		}
		if (look->animal)
			*dest = look->animal->brain->signature;
		else
			set_error(self, FEMPTY);
	} break;
	case OP_BABY: {
		uint16_t direction, energy;
		if (read_from(self, instr.l_fmt, instr.left, &direction)
		 || read_from(self, instr.r_fmt, instr.right, &energy))
			goto error;
		struct tile *targ = in_direction(g, direction, x, y);
		if ((ptrdiff_t)targ == -1) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		if (!targ) {
			set_error(self, FBLOCKED);
			goto error;
		}
		add_saturate(&energy, self->brain->ram_size);
		uint8_t codea = self->brain->code_size >> 3,
			codeb = self->brain->code_size >> 11;
		if (energy >= self->energy
		 || codea > self->stomach[CHEM_CODEA]
		 || codeb > self->stomach[CHEM_CODEB]) {
			set_error(self, FEMPTY);
			goto error;
		}
		self->energy -= energy;
		self->stomach[CHEM_CODEA] -= codea;
		self->stomach[CHEM_CODEB] -= codeb;
		if (grid_next_mutant(g))
			targ->animal = animal_mutant(self->brain, energy - self->brain->ram_size, g);
		else
			targ->animal = animal_new(self->brain, energy - self->brain->ram_size);
		targ->animal->health = g->health;
		targ->animal->lifetime = g->lifetime;
	} break;
	case OP_STEP: {
		uint16_t direction;
		if (read_from(self, instr.l_fmt, instr.left, &direction))
			goto error;
		struct tile *dest = in_direction(g, direction, x, y);
		if ((ptrdiff_t)dest == -1) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		if (!dest) {
			set_error(self, FBLOCKED);
			goto error;
		}
		if (dest->animal) {
			set_error(self, FBLOCKED);
			goto error;
		}
		if (!dest->animal) {
			tile_set_animal(dest, self);
			grid_get_unck(g, x, y)->animal = NULL;
		}
	} break;
	case OP_ATTK: {
		uint16_t direction, power;
		if (read_from(self, instr.l_fmt, instr.left, &direction)
		 || read_from(self, instr.r_fmt, instr.right, &power))
			goto error;
		struct tile *targ = in_direction(g, direction, x, y);
		if ((ptrdiff_t)targ == -1) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		if (!targ) {
			set_error(self, FBLOCKED);
			goto error;
		}
		if (!targ->animal) {
			set_error(self, FEMPTY);
			goto error;
		}
		sub_saturate(&self->energy, power / 2);
		sub_saturate(&targ->animal->health, power);
	} break;
	case OP_CONV: {
		uint16_t c1, c2;
		if (read_from(self, instr.l_fmt, instr.left, &c1)
		 || read_from(self, instr.r_fmt, instr.right, &c2))
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
			set_error(self, FEMPTY);
			goto error;
		}
	} break;
	case OP_EAT: {
		uint16_t chem, amount;
		if (read_from(self, instr.l_fmt, instr.left, &chem)
		 || read_from(self, instr.r_fmt, instr.right, &amount))
			goto error;
		if (chem >= N_CHEMICALS) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		amount &= UINT8_MAX;
		if (amount > self->stomach[chem]) {
			set_error(self, FEMPTY);
			amount = self->stomach[chem];
		}
		self->stomach[chem] -= amount;
		add_saturate(&self->energy, amount * chemical_table[chem].energy);
	} break;
	case OP_GCHM: {
		uint16_t chem, *dest = write_dest(self, instr.l_fmt, instr.left);
		if (!dest || read_from(self, instr.r_fmt, instr.right, &chem))
			goto error;
		if (chem >= N_CHEMICALS) {
			set_error(self, FINVAL_ARG);
			goto error;
		}
		*dest = self->stomach[chem];
	} break;
	case OP_GHLT: {
		uint16_t *dest = write_dest(self, instr.l_fmt, instr.left);
		if (dest)
			*dest = self->health;
		else
			goto error;
	} break;
	case OP_GNRG: {
		uint16_t *dest = write_dest(self, instr.l_fmt, instr.left);
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
	sub_saturate(&self->energy, op_info[instr.opcode].energy);
error:
	++self->instr_ptr;
	--self->lifetime;
}

struct animal *animal_new(struct brain *brain, uint16_t energy)
{
	struct animal *self = malloc(sizeof(struct animal) + brain->ram_size * sizeof(uint16_t));
	++brain->refcount;
	self->brain = brain;
	self->energy = energy;
	self->instr_ptr = 0;
	self->flags = 0;
	memset(self->stomach, 0, N_CHEMICALS);
	memset(self->ram, 0, brain->ram_size * sizeof(uint16_t));
	return self;
}

struct animal *animal_mutant(struct brain *brain, uint16_t energy, struct grid *g)
{
	return animal_new(brain_mutate(brain, g), energy);
}

bool animal_is_dead(const struct animal *self)
{
	return self->lifetime == 0 || self->energy == 0 || self->health == 0;
}

void animal_spill_guts(const struct animal *self, struct tile *t)
{
	for (size_t i = 0; i < N_CHEMICALS; ++i)
		if (UINT8_MAX - t->chemicals[i] < self->stomach[i])
			t->chemicals[i] = UINT8_MAX;
		else
			t->chemicals[i] += self->stomach[i];
}

void animal_free(struct animal *self)
{
	--self->brain->refcount;
	free(self);
}
