/*
 * The interface for writing to and reading from save files.
 *
 * Copyright (C) 2018 Jude Melton-Houghton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * */

#ifndef _SAVE_H

#define _SAVE_H

#include "grid.h"
#include <stdio.h>

#if N_CHEMICALS != 11
	#error "Be sure to change the version number when changing N_CHEMICALS!"
#endif
#define SERIALIZATION_VERSION 3

int write_grid(struct grid *g, FILE *dest, const char **err);

struct grid *read_grid(FILE *src, const char **err);

#endif /* Header guard */
