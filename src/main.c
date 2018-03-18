#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include <stdio.h>

int main(void)
{
	struct grid *g = grid_new(20, 20);
	struct brain *b = brain_new(0xdead, 128, 128);
	struct animal *a = animal_new(b, 10000, 10000, 10000);
}

struct brain *brain_new(uint16_t signature, uint16_t ram_size, uint16_t code_size);

struct animal *animal_new(struct brain *brain, uint16_t health, uint16_t energy, uint16_t lifetime);

void animal_step(struct animal *self);

bool animal_is_dead(struct animal *self);

void animal_free(struct animal *self);

struct grid;
void animal_act(struct animal *self, struct grid *grid, size_t x, size_t y);
