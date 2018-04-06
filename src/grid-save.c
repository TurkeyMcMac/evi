/*
 * The code for writing grids to and reading grids from save files.
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

#include "save.h"

#include "animal.h"
#include "grid.h"
#include <arpa/inet.h>
#include <stdlib.h>

#define STORED_TILE_SIZE (sizeof(uint32_t) + N_CHEMICALS)

#define RETURN_ERR (-1)
static int write_tile(const struct tile *t,
	uint32_t animal_off,
	FILE *dest,
	const char **err)
{
	animal_off = htonl(animal_off);
	FWRITE(&animal_off, sizeof(animal_off), 1, dest, err);
	FWRITE(t->chemicals, sizeof(*t->chemicals), N_CHEMICALS, dest, err);
	return 0;
}

int grid_write(struct grid *g, FILE *dest, const char **err)
{
	uint32_t version = htonl(SERIALIZATION_VERSION);
	FWRITE(&version, sizeof(version), 1, dest, err);

	uint16_t fields16[3] = {
		htons(g->tick), htons(g->drop_interval), htons(g->health)
	};
	FWRITE(fields16, sizeof(*fields16), 3, dest, err);
	uint32_t fields32[2] = {htonl(g->random), htonl(g->mutate_chance)};
	FWRITE(fields32, sizeof(*fields32), 2, dest, err);
	FWRITE(&g->drop_amount, sizeof(g->drop_amount), 1, dest, err);

	long n_species_off;
	FTELL(&n_species_off, dest, err);
	uint32_t n_species;
	FSEEK(dest, sizeof(n_species), SEEK_CUR, err);
	struct brain *b;
	n_species = 0;
	SLLIST_FOR_EACH(g->species, b) {
		b->save_num = htonl(n_species++);
		if (brain_write(b, dest, err))
			return -1;
	}
	n_species = htonl(n_species);
	long species_end;
	FTELL(&species_end, dest, err);
	if (fseek(dest, n_species_off, SEEK_SET))
		FAIL(fseek, err);
	FWRITE(&n_species, sizeof(n_species), 1, dest, err);
	if (fseek(dest, species_end, SEEK_SET))
		FAIL(fseek, err);

	uint32_t dimensions[2] = {htonl(g->width), htonl(g->height)};
	FWRITE(dimensions, sizeof(*dimensions), 2, dest, err);
	long next_tile, next_animal;
	FTELL(&next_tile, dest, err);
	next_animal = next_tile + g->width * g->height * STORED_TILE_SIZE;
	for (size_t i = 0; i < g->width * g->height; ++i) {
		const struct tile *t = &g->tiles[i];
		if (t->animal) {
			if (write_tile(t, next_animal - next_tile, dest, err))
				return -1;
			next_tile += STORED_TILE_SIZE;
			FSEEK(dest, next_animal, SEEK_SET, err);
			if (animal_write(t->animal, dest, err))
				return -1;
			FTELL(&next_animal, dest, err);
			FSEEK(dest, next_tile, SEEK_SET, err);
		} else {
			if (write_tile(t, 0, dest, err))
				return -1;
			next_tile += STORED_TILE_SIZE;
		}
	}
	return 0;
}
#undef RETURN_ERR

#define RETURN_ERR NULL
/* TODO: Fix all the possible memory leaks here. */
struct grid *grid_read(FILE *src, const char **err)
{
	uint32_t version;
	FREAD(&version, sizeof(version), 1, src, err);
	if (ntohl(version) != SERIALIZATION_VERSION) {
		errno = EPROTONOSUPPORT; /* I don't think that this is what
					  * EPROTONOSUPPORT is meant for, but
					  * it's close enough. */
		*err = "format version mismatch";
		return NULL;
	}

	/* These fields will be converted to host format later. */
	uint16_t fields16[3];
	uint32_t fields32[2];
	uint8_t drop_amount;
	FREAD(fields16, sizeof(*fields16), 3, src, err);
	FREAD(fields32, sizeof(*fields32), 2, src, err);
	FREAD(&drop_amount, sizeof(drop_amount), 1, src, err);

	uint32_t n_species;
	FREAD(&n_species, sizeof(n_species), 1, src, err);
	n_species = ntohl(n_species);
	struct brain **species = calloc(n_species, sizeof(struct brain *));
	if (!species) {
		*err = "too many species";
		return NULL;
	}
	for (uint32_t i = 0; i < n_species; ++i) {
		struct brain *b = brain_read(src, err);
		if (!b)
			return NULL;
		species[i] = b;
	}
	uint32_t dims[2];
	FREAD(dims, sizeof(*dims), 2, src, err);
	// TODO: Use a function with less built-in initialization.
	struct grid *g = grid_new(ntohl(dims[0]), ntohl(dims[1]));
	g->tick = ntohs(fields16[0]);
	g->drop_interval = ntohs(fields16[1]);
	g->health = ntohs(fields16[2]);
	g->random = ntohl(fields32[0]);
	g->mutate_chance = ntohl(fields32[1]);
	g->drop_amount = drop_amount;

	long next_tile = ftell(src);
	if (next_tile == -1)
		return NULL;
	for (size_t i = 0; i < g->width * g->height; ++i) {
		uint32_t animal;
		FREAD(&animal, sizeof(animal), 1, src, err);
		FREAD(g->tiles[i].chemicals, sizeof(*g->tiles[i].chemicals),
			N_CHEMICALS, src, err);
		next_tile += STORED_TILE_SIZE;
		if (animal) {
			FSEEK(src,
				ntohl(animal) - STORED_TILE_SIZE,
				SEEK_CUR, err);
			struct animal *a =
				animal_read(species, n_species, src, err);
			if (!a) {
				return NULL;
			}
			g->tiles[i].animal = a;
			FSEEK(src, next_tile, SEEK_SET, err);
		} else
			g->tiles[i].animal = NULL;
	}

	for (size_t i = 0; i < n_species; ++i) {
		species[i]->next = g->species;
		g->species = species[i];
	}
	free(species);
	return g;
}
