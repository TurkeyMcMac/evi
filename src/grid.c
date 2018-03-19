#include "grid.h"
#include <stdlib.h>

struct grid *grid_new(size_t width, size_t height)
{
	struct grid *self = calloc(1, sizeof(struct grid) + width * height * sizeof(struct tile));
	self->species = NULL;
	self->animals = NULL;
	self->tick = 0;
	self->width = width;
	self->height = height;
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
	int r = t->chemicals[CHEM_RED  ],
	    g = t->chemicals[CHEM_GREEN],
	    b = t->chemicals[CHEM_BLUE ];
	if (r) {
		r /= 51;
		r *= 18;
		r += 72;
	}
	if (g) {
		g /= 51;
		g *= 3;
		g += 12;
	}
	if (b) {
		b /= 102;
		b += 2;
	}
	fprintf(dest, "\x1B[48;5;%dm", r + g + b + 16);
}

void grid_draw(const struct grid *self, FILE *dest)
{
	size_t x, y;
	for (y = 0; y < self->height; ++y) {
		for (x = 0; x < self->width; ++x) {
			const struct tile *t = grid_get_const_unck(self, x, y);
			print_color(t, dest);
			if (t->animal)
				printf("<>");	
			else
				printf("  ");
		}
		printf("\x1B[0m\n");
	}
}

#define SLLIST_FOR_EACH(list, item, last) for ((item) = (list), (last) = &(list); (item) != NULL; \
		(last) = &(item)->next, (item) = (item)->next)

static void step_animals(struct grid *g)
{
	struct animal *a, **last_a;
	SLLIST_FOR_EACH (g->animals, a, last_a)
		if (animal_die(a))
			*last_a = a->next;
		else
			animal_step(a);
}

static void init_fluid_mask(struct grid *g)
{
	for (size_t i = 0; i < N_CHEMICALS; ++i) {
		g->flowing |= (g->tick % chemical_table[i].flow == 0) << i;
	}
}

static void flow_fluids(struct grid *g, struct tile *t, size_t x, size_t y)
{
	for (uint16_t idx, flowing = g->flowing; flowing != 0; flowing &= flowing - 1) {
		idx = __builtin_ctz(flowing);
		if (t->chemicals[idx] >= 4) {
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

static void update_tiles(struct grid *g)
{
	init_fluid_mask(g);
	size_t x, y;
	for (y = 0; y < g->height; ++y)
		for (x = 0; x < g->width; ++x) {
			struct tile *t = grid_get_unck(g, x, y);
			if (t->animal) {
				if (t->animal->health > 0)
					animal_act(t->animal, g, x, y);
				else {
					animal_free(t->animal);
					t->animal = NULL;
				}
			}
			flow_fluids(g, t, x, y);
		}
	g->flowing = 0;
}

static void free_extinct(struct grid *g)
{
	struct brain *b, **last_b;
	SLLIST_FOR_EACH (g->species, b, last_b)
		if (b->refcount == 0) {
			*last_b = b->next;
			free(b);
		}
}

void grid_update(struct grid *self)
{
	step_animals(self);
	update_tiles(self);
	free_extinct(self);
	++self->tick;
}

void grid_free(struct grid *self)
{
	struct animal *a = self->animals;
	while (a != NULL) {
		struct animal *next = a->next;
		free(a);
		a = next;
	}
	struct brain *b = self->species;
	while (b != NULL) {
		struct brain *next = b->next;
		free(b);
		b = next;
	}
	free(self);
}
