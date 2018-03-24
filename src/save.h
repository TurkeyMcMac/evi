#ifndef _SAVE_H

#define _SAVE_H

#include "grid.h"
#include <stdio.h>

int write_grid(struct grid *g, FILE *dest, const char **err);

#endif /* Header guard */
