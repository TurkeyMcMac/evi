#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include <stdio.h>

int main(void)
{
	struct grid *g = grid_new(7, 7);
	uint8_t *chem = grid_get(g, 6, 4)->chemicals;
	chem[CHEM_RED]   = 254;
	chem[CHEM_GREEN] =  24;
	chem[CHEM_BLUE]  = 100;
	grid_draw(g, stdout);
}
