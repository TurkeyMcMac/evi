#include "grid.h"
#include <stdlib.h>

struct grid *grid_new(size_t width, size_t height)
{
	struct grid *g = calloc(1, sizeof(struct grid) + width * height * sizeof(struct tile));
	g->width = width;
	g->height = height;
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
