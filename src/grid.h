#ifndef _GRID_H

#define _GRID_H

#include "chemicals.h"
#include <stddef.h>
#include <stdio.h>

struct tile {
	uint8_t chemicals[N_CHEMICALS];
};

struct grid {
	size_t width, height;
	struct tile tiles[];
};

struct grid *grid_new(size_t width, size_t height);

struct tile *grid_get_unck(struct grid *self, size_t x, size_t y);

struct tile *grid_get(struct grid *self, size_t x, size_t y);

const struct tile *grid_get_const_unck(const struct grid *self, size_t x, size_t y);

const struct tile *grid_get_const(const struct grid *self, size_t x, size_t y);

void grid_draw(const struct grid *self, FILE *dest);

#endif /* Header guard */
