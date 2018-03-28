#ifndef _SAVE_H

#define _SAVE_H

#include "grid.h"
#include <stdio.h>

#if N_CHEMICALS != 11
	#error "Be sure to change the version number when changing N_CHEMICALS!"
#endif
#define SERIALIZATION_VERSION 2

int write_grid(struct grid *g, FILE *dest, const char **err);

struct grid *read_grid(FILE *src, const char **err);

#endif /* Header guard */
