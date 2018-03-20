#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const struct instruction code[] = {
	[0x0000] = {OP_STEP, 1,0, 0x0000},
	[0x0001] = {OP_JPNO, 0,0, 0x0000, FBLOCKED},
	[0x0002] = {OP_INCR, 1,0, 0x0000},
	[0x0003] = {OP_AND , 1,0, 0x0000, 3},
	[0x0004] = {OP_JUMP, 0,0, 0x0000},
};

int main(void)
{
	struct grid *g = grid_new(21, 21);
	struct brain *b = brain_new(0xdead, 128, 128);
	memcpy(b->code, code, sizeof(code));
	struct animal *a = animal_new(b, 10000, 10000, 10000);
	struct animal *a1 = animal_new(b, 10000, 10000, 10000);
	g->species = b;
	g->animals = a;
	a->next = a1;
	grid_get(g, 0, 20)->animal = a;
	grid_get(g, 0,  0)->animal = a1;
	struct tile *rspring = grid_get(g, 7, 2),
		    *gspring = grid_get(g, 8, 5),
		    *bspring = grid_get(g, 7, 6);
	a1->ram[0] = 2;
	while (true) {
		grid_draw(g, stdout);
		grid_update(g);
		rspring->chemicals[CHEM_BLUE] = UINT8_MAX;
		bspring->chemicals[CHEM_RED] = UINT8_MAX;
		gspring->chemicals[CHEM_GREEN] = UINT8_MAX;
		usleep(100000);
	}
	grid_free(g);
}
