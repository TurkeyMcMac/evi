/*
 * The code for writing to and reading from save files.
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

#include "animal.h"
#include "save.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define RETURN_ERR (-1)
int animal_write(const struct animal *a, FILE *dest, const char **err)
{
	FWRITE(&a->brain->save_num, sizeof(a->brain->save_num), 1, dest, err);
	uint16_t fields16[4] = {
		htons(a->health), htons(a->energy), htons(a->instr_ptr),
		htons(a->flags)
	};
	FWRITE(fields16, sizeof(*fields16), 4, dest, err);
	FWRITE(a->stomach, sizeof(*a->stomach), N_CHEMICALS, dest, err);
	for (uint16_t i = 0; i < a->brain->ram_size; ++i) {
		uint16_t cell = htons(a->ram[i]);
		FWRITE(&cell, sizeof(cell), 1, dest, err);
	}
	return 0;
}
#undef RETURN_ERR

#define RETURN_ERR NULL
struct animal *animal_read(struct brain **species,
		uint32_t n_species,
		FILE *src,
		const char **err)
{
	uint32_t brain_num;
	FREAD(&brain_num, sizeof(brain_num), 1, src, err);
	brain_num = ntohl(brain_num);
	if (brain_num >= n_species) {
		errno = ENODATA;
		*err = "species number too high";
		return NULL;
	}
	struct brain *b = species[brain_num];
	++b->refcount;
	struct animal *a = malloc(offsetof(struct animal, ram)
		+ b->ram_size * sizeof(uint16_t));
	a->brain = b;
	uint16_t fields16[4];
	FREAD(fields16, sizeof(*fields16), 4, src, err);
	a->health = ntohs(fields16[0]);
	a->energy = ntohs(fields16[1]);
	a->instr_ptr = ntohs(fields16[2]);
	a->flags = ntohs(fields16[3]);
	FREAD(a->stomach, sizeof(*a->stomach), N_CHEMICALS, src, err);
	for (uint16_t i = 0; i < b->ram_size; ++i) {
		FREAD(&a->ram[i], sizeof(a->ram[i]), 1, src, err);
		a->ram[i] = ntohs(a->ram[i]);
	}
	return a;
}
