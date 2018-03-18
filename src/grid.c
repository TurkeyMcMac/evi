#include "grid.h"
#include <stdlib.h>

struct grid *grid_new(size_t width, size_t height)
{
	struct grid *self = calloc(1, sizeof(struct grid) + width * height * sizeof(struct tile));
	self->species = NULL;
	self->animals = NULL;
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

#define color(r, g, b) ((r) / 51 * 36 + (g) / 51 * 6 + (b) / 51 + 16)

void grid_draw(const struct grid *self, FILE *dest)
{
	size_t x, y;
	for (y = 0; y < self->height; ++y) {
		for (x = 0; x < self->width; ++x) {
			const struct tile *t = grid_get_const_unck(self, x, y);
			char icon;
			if (t->animal)
				icon = '$';
			else
				icon = ' ';
			printf("\x1B[48;5;%dm%c", color(t->chemicals[CHEM_RED],
						t->chemicals[CHEM_GREEN],
						t->chemicals[CHEM_BLUE]),
					icon);
		}
		printf("\x1B[0m\n");
	}
}

#define SLLIST_FOR_EACH(list, item, last) for ((item) = (list), (last) = &(list); (item) != NULL; \
		(last) = &(item)->next, (item) = (item)->next)

void grid_update(struct grid *self)
{
	struct animal *a, **last_a;
	SLLIST_FOR_EACH (self->animals, a, last_a)
		if (animal_die(a))
			*last_a = a->next;
		else
			animal_step(a);
	size_t x, y;
	for (y = 0; y < self->height; ++y)
		for (x = 0; x < self->width; ++x) {
			struct tile *t = grid_get_unck(self, x, y);
			if (t->animal) {
				if (t->animal->health > 0)
					animal_act(t->animal, self, x, y);
				else {
					animal_free(t->animal);
					t->animal = NULL;
				}
			}
		}
	struct brain *b, **last_b;
	SLLIST_FOR_EACH (self->species, b, last_b)
		if (b->refcount == 0) {
			*last_b = b->next;
			free(b);
		}
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
