#include "grid.h"
#include <stdlib.h>

struct grid *grid_new(size_t width, size_t height, uint16_t health, uint16_t lifetime)
{
	struct grid *self = calloc(1, sizeof(struct grid) + width * height * sizeof(struct tile));
	self->species = NULL;
	self->animals = NULL;
	self->tick = 0;
	self->width = width;
	self->height = height;
	self->health = health;
	self->lifetime = lifetime;
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

#define HAS_SPRING (1 << 7)

static void flow_fluids(uint16_t flowing, struct grid *g, struct tile *t, size_t x, size_t y)
{
	uint16_t idx;
	for (; flowing != 0; flowing &= flowing - 1) {
		idx = __builtin_ctz(flowing);
		if (t->spring & HAS_SPRING) {
			t->chemicals[t->spring & ~HAS_SPRING] = UINT8_MAX;
		}
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
			if (t->animal) {
				if (t->animal->health > 0)
					animal_act(t->animal, g, x, y);
				else {
					animal_free(t->animal);
					t->animal = NULL;
				}
			}
			flow_fluids(flowing, g, t, x, y);
			evaporate_fluids(evaporating, t);
		}
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

void grid_random_springs(struct grid *self,
		size_t n_chems,
		const enum chemical chems[],
		size_t n_springs)
{
	while (n_springs--) {
		size_t x = rand() % self->width,
		       y = rand() % self->height;
		printf("(%lu, %lu)\n", x, y);
		grid_get_unck(self, x, y)->spring = HAS_SPRING | chems[rand() % n_chems];
	}
}
