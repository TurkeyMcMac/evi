#ifndef _GRID_H

#define _GRID_H

#include "animal.h"
#include "chemicals.h"
#include <stddef.h>
#include <stdio.h>

struct tile {
	struct animal *animal;
	uint8_t chemicals[N_CHEMICALS];
};

struct grid {
	struct brain *species;
	struct animal *animals;
	uint16_t tick, drop_interval;
	uint16_t health, lifetime;
	uint32_t random;
	uint32_t mutate_chance;
	uint8_t drop_amount;
	size_t width, height;
	struct tile tiles[];
};

struct grid *grid_new(size_t width, size_t height);

struct tile *grid_get_unck(struct grid *self, size_t x, size_t y);

struct tile *grid_get(struct grid *self, size_t x, size_t y);

void grid_add_animal(struct grid *self, struct animal *a);

const struct tile *grid_get_const_unck(const struct grid *self, size_t x, size_t y);

const struct tile *grid_get_const(const struct grid *self, size_t x, size_t y);

void grid_draw(const struct grid *self, FILE *dest);

uint32_t grid_rand(struct grid *self);

bool grid_next_mutant(struct grid *self);

void grid_update(struct grid *self);

void grid_free(struct grid *self);

#endif /* Header guard */
