#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DIRECT 0x0000
#define NOSE 0x0001

static const struct instruction code[] = {
	[0x0000] = {OP_PICK, 1,0, 4, (255 << 8) | CHEM_ENERGY},
	[0x0001] = {OP_EAT, 0,0, CHEM_ENERGY, 255},
	[0x0002] = {OP_STEP, 1,0, DIRECT},
	[0x0003] = {OP_JPNO, 0,0, 0x0000, FBLOCKED},
	[0x0004] = {OP_INCR, 1,0, DIRECT},
	[0x0005] = {OP_AND , 1,0, DIRECT, 3},
	[0x0006] = {OP_JUMP, 0,0, 0x0000},
};

int main(void)
{
	struct grid *g = grid_new(21, 21, 50, 5000);
	struct brain *b = brain_new(0xdead, 5, sizeof(code));
	memcpy(b->code, code, sizeof(code));
	struct animal *a = animal_new(b, 50, 1000, 5000);
	struct animal *a1 = animal_new(b, 50, 1000, 10000);
	g->species = b;
	g->animals = a;
	a->next = a1;
	grid_get(g, 0, 20)->animal = a;
	grid_get(g, 0,  0)->animal = a1;
	struct tile *rspring = grid_get(g, 0, 0),
		    *gspring = grid_get(g, 0,20),
		    *bspring = grid_get(g,20, 0);
	a1->ram[0] = 2;
	while (true) {
		grid_draw(g, stdout);
		grid_update(g);
		rspring->chemicals[CHEM_ENERGY] = UINT8_MAX;
		bspring->chemicals[CHEM_ENERGY] = UINT8_MAX;
		gspring->chemicals[CHEM_ENERGY] = UINT8_MAX;
		usleep(10000);
	}
	grid_free(g);
}
