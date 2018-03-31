#ifndef _ANIMAL_H

#define _ANIMAL_H

#include "brain.h"
#include "chemicals.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct animal {
	struct brain *brain;
	uint16_t health;
	uint16_t energy;
	uint16_t lifetime;
	uint16_t instr_ptr;
	uint16_t flags;
	uint8_t stomach[N_CHEMICALS];
	uint16_t ram[];
};

struct animal *animal_new(struct brain *brain, uint16_t energy);

struct animal *animal_mutant(struct brain *brain, uint16_t energy, struct grid *g);

void animal_step(struct animal *self, struct grid *grid, size_t x, size_t y);

bool animal_is_dead(const struct animal *self);

struct tile;

void animal_spill_guts(const struct animal *self, struct tile *t);

void animal_free(struct animal *self);

#endif /* Header guard */
