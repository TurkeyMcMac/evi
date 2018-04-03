/*
 * The code for interacting with the grid.
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

#include "grid.h"

#include "random.h"
#include <stdlib.h>

void tile_set_animal(struct tile *self, struct animal *a)
{
	self->animal = a;
	self->newly_occupied = true;
}

struct grid *grid_new(size_t width, size_t height)
{
	struct grid *self = calloc(1, offsetof(struct grid, tiles) +
		width * height * sizeof(struct tile));
	self->width = width;
	self->height = height;
	self->drop_interval = 1;
	return self;
}

struct tile *grid_get_unck(struct grid *self, size_t x, size_t y)
{
	return &self->tiles[y * self->width + x];
}

struct tile *grid_get(struct grid *self, size_t x, size_t y)
{
	if (x < self->width && y < self->height)
		return grid_get_unck(self, x, y);
	else
		return NULL;
}

const struct tile *grid_get_const_unck(const struct grid *self, size_t x, size_t y)
{
	return &self->tiles[y * self->width + x];
}

const struct tile *grid_get_const(const struct grid *self, size_t x, size_t y)
{
	if (x < self->width && y < self->height)
		return grid_get_const_unck(self, x, y);
	else
		return NULL;
}

static void print_color(const struct tile *t, FILE *dest)
{
	int r = t->chemicals[CHEM_RED],
	    g = t->chemicals[CHEM_GREEN],
	    b = t->chemicals[CHEM_BLUE];
	if (r) {
		r /= 51;
		if (r > 4)
			r = 4;
		r *= 36;
		r += 36;
	}
	if (g) {
		g /= 51;
		if (g > 4)
			g = 4;
		g *= 6;
		g += 6;
	}
	if (b) {
		b /= 51;
		if (b > 4)
			b = 4;
		b += 1;
	}
	fprintf(dest, "\x1B[48;5;%dm", r + g + b + 16);
}

#define EMPTY_GRAY "237"

void grid_draw(const struct grid *self, FILE *dest)
{
	fprintf(dest, "\x1B[38;5;"EMPTY_GRAY"m");
	size_t x, y;
	for (y = 0; y < self->height; ++y) {
		for (x = 0; x < self->width; ++x) {
			const struct tile *t = grid_get_const_unck(self, x, y);
			print_color(t, dest);
			if (t->animal)
				fprintf(dest, "\x1B[37m[]\x1B[38;5;"EMPTY_GRAY"m");
			else
				fprintf(dest, "[]");
		}
		fprintf(dest, "\x1B[49m\n");
	}
	fprintf(dest, "\x1B[39;49m");
	fflush(dest);
}

void grid_print_species(const struct grid *self, size_t threshold, FILE *dest)
{
	const struct brain *b;
	SLLIST_FOR_EACH(self->species, b) {
		if (b->refcount >= threshold)
			brain_print(b, dest);
	}
}

static uint16_t init_flow_mask(uint16_t tick)
{
	uint16_t flowing = 0;
	for (size_t i = 0; i < N_CHEMICALS; ++i) {
		flowing |= (tick % chemical_table[i].flow == 0) << i;
	}
	return flowing;
}

static uint16_t init_evaporation_mask(uint16_t tick)
{
	uint16_t evaporating = 0;
	for (size_t i = 0; i < N_CHEMICALS; ++i) {
		evaporating |= (tick % chemical_table[i].evaporation == 0) << i;
	}
	return evaporating;
}

static void flow_fluids(uint16_t flowing, struct grid *g, struct tile *t, size_t x, size_t y)
{
	uint16_t idx;
	for (; flowing != 0; flowing &= flowing - 1) {
		idx = __builtin_ctz(flowing);
		if (t->chemicals[idx] > 4) {
			struct tile *flow_to;
			if ((flow_to = grid_get(g, x, y - 1)) != NULL
			 && flow_to->chemicals[idx] != UINT8_MAX) {
				++flow_to->chemicals[idx];
				--t->chemicals[idx];
			}
			if ((flow_to = grid_get(g, x + 1, y)) != NULL
			 && flow_to->chemicals[idx] != UINT8_MAX) {
				++flow_to->chemicals[idx];
				--t->chemicals[idx];
			}
			if ((flow_to = grid_get(g, x, y + 1)) != NULL
			 && flow_to->chemicals[idx] != UINT8_MAX) {
				++flow_to->chemicals[idx];
				--t->chemicals[idx];
			}
			if ((flow_to = grid_get(g, x - 1, y)) != NULL
			 && flow_to->chemicals[idx] != UINT8_MAX) {
				++flow_to->chemicals[idx];
				--t->chemicals[idx];
			}
		}
	}
}

static void evaporate_fluids(uint16_t evaporating, struct tile *t)
{
	uint16_t idx;
	for (; evaporating != 0; evaporating &= evaporating - 1) {
		idx = __builtin_ctz(evaporating);
		if (t->chemicals[idx] > 0)
			--t->chemicals[idx];
	}
}

static void update_tiles(struct grid *g)
{
	uint16_t flowing = init_flow_mask(g->tick),
		 evaporating = init_evaporation_mask(g->tick);
	size_t x, y;
	for (y = 0; y < g->height; ++y)
		for (x = 0; x < g->width; ++x) {
			struct tile *t = grid_get_unck(g, x, y);
			struct animal *a = t->animal;
			if (a && !t->newly_occupied) {
				if (animal_is_dead(a)) {
					animal_spill_guts(a, t);
					animal_free(a);
					t->animal = NULL;
				} else
					animal_step(a, g, x, y);
			}
			t->newly_occupied = false;
			flow_fluids(flowing, g, t, x, y);
			evaporate_fluids(evaporating, t);
		}
}

static void free_extinct(struct grid *g)
{
	struct brain *b, **last_b = &g->species;
	for (b = g->species, last_b = &g->species; b != NULL; )
		if (b->refcount == 0) {
			struct brain *next = b->next;
			*last_b = next;
			free(b);
			b = next;
		} else {
			last_b = &b->next;
			b = b->next;
		}
}

uint32_t grid_rand(struct grid *self)
{
	return self->random = randomize(self->random);
}

bool grid_next_mutant(struct grid *self)
{
	return grid_rand(self) < self->mutate_chance;
}

void grid_update(struct grid *self)
{
	update_tiles(self);
	free_extinct(self);
	if (self->tick % self->drop_interval == 0) {
		grid_get_unck(self, grid_rand(self) % self->width, grid_rand(self) % self->height)
			->chemicals[grid_rand(self) % 3 + 1] = self->drop_amount;
	}
	++self->tick;
}

void grid_free(struct grid *self)
{
	for (size_t i = 0; i < self->width * self->height; ++i) {
		if (self->tiles[i].animal)
			animal_free(self->tiles[i].animal);
	}
	struct brain *b = self->species;
	while (b != NULL) {
		struct brain *next = b->next;
		free(b);
		b = next;
	}
	free(self);
}
