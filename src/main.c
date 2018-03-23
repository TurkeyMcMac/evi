#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define DIRECT 0x0000
#define NOSE 0x0001

static const struct instruction code[] = {
	[0x0000] = {OP_PICK, 1,0, 4, (255 << 8) | CHEM_RED},
	[0x0001] = {OP_PICK, 1,0, 4, (255 << 8) | CHEM_GREEN},
	[0x0002] = {OP_PICK, 1,0, 4, (255 << 8) | CHEM_BLUE},
	[0x0003] = {OP_CONV, 0,0, CHEM_GREEN, CHEM_BLUE}, /* Cyan */
	[0x0004] = {OP_CONV, 0,0, CHEM_RED, CHEM_CYAN},   /* Energy */
	[0x0005] = {OP_STEP, 1,0, DIRECT},
	[0x0006] = {OP_JPNO, 0,0, 0x0000, FBLOCKED},
	[0x0007] = {OP_ADD,  1,0, DIRECT, 2},
	[0x0008] = {OP_AND , 1,0, DIRECT, 3},
	[0x0009] = {OP_EAT,  0,0, CHEM_ENERGY, 255},
	[0x000A] = {OP_JUMP, 0,0, 0x0000},
};

static const enum chemical spring_colors[] = {CHEM_RED, CHEM_GREEN, CHEM_BLUE};

int main(void)
{
	srand(time(NULL));
	struct grid *g = grid_new(50, 50, 50, 5000);
	grid_random_springs(g, 3, spring_colors, 40);
	struct brain *b = brain_new(0xdead, 5, sizeof(code));
	memcpy(b->code, code, sizeof(code));
	struct animal *a  = animal_new(b, 50, 10000, -1);
	struct animal *a1 = animal_new(b, 50, 10000, -1);
	g->species = b;
	g->animals = a;
	a->next = a1;
	grid_get(g, 7, 10)->animal = a;
	grid_get(g, 12, 10)->animal = a1;
	a1->ram[0] = 2;
	while (true) {
		size_t i;
		grid_draw(g, stdout);
		grid_update(g);
		usleep(20000);
	}
	grid_free(g);
}
