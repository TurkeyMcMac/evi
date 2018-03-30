#include "save.h"

#include "animal.h"
#include "grid.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define FAIL(fn, e) do { *(e) = #fn " failed"; return -1; } while (0)

#define FWRITE(src, size, nmemb, dest, e) do { \
	if (fwrite((src), (size), (nmemb), (dest)) != (nmemb)) \
		FAIL(fwrite, (e)); \
} while (0)

#define FTELL(dest, src, e) do { \
	*(dest) = ftell(src); \
	if (*(dest) == -1) \
		FAIL(ftell, (e)); \
} while (0)

#define FSEEK(f, location, whence, e) do { \
	if (fseek((dest), (location), (whence))) \
		FAIL(fseek, (e)); \
} while (0)

#define STORED_TILE_SIZE (sizeof(uint32_t) + N_CHEMICALS)

static int write_brain(const struct brain *b, FILE *dest, const char **err);
static int write_tile(const struct tile *t, uint32_t animal_off, FILE *dest, const char **err);
static int write_animal(const struct animal *a, FILE *dest, const char **err);

int write_grid(struct grid *g, FILE *dest, const char **err)
{
	uint32_t version = htonl(SERIALIZATION_VERSION);
	FWRITE(&version, sizeof(version), 1, dest, err);

	uint16_t fields16[4] = {
		htons(g->tick), htons(g->drop_interval), htons(g->health), htons(g->lifetime)
	};
	FWRITE(fields16, sizeof(*fields16), 4, dest, err);
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
		if (write_brain(b, dest, err))
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
			if (write_animal(t->animal, dest, err))
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

#define FAIL_PTR(fn, e) do { *(e) = #fn " failed"; return NULL; } while (0)

#define FREAD(dest, size, nmemb, src, e) do { \
	if (fread((dest), (size), (nmemb), (src)) != (nmemb)) { \
		if (feof(src)) { \
			errno = EPROTO; \
			*(e) = "unexpected end of file"; \
			return NULL; \
		} \
		FAIL_PTR(fread, (e)); \
	} \
} while (0)


static struct brain *read_brain(FILE *src, const char **err);
static struct animal *read_animal(struct brain **species,
		uint32_t n_species,
		FILE *src,
		const char **err);

/* TODO: Fix all the possible memory leaks here. */
struct grid *read_grid(FILE *src, const char **err)
{
	uint32_t version;
	FREAD(&version, sizeof(version), 1, src, err);
	if (ntohl(version) != SERIALIZATION_VERSION) {
		errno = EPROTONOSUPPORT; /* I don't think that this is what EPROTONOSUPPORT is
					    meant for, but it's close enough. */
		*err = "format version mismatch";
		return NULL;
	}

	/* These fields will be converted to host format later. */
	uint16_t fields16[4];
	uint32_t fields32[2];
	uint8_t drop_amount;
	FREAD(fields16, sizeof(*fields16), 4, src, err);
	FREAD(fields32, sizeof(*fields32), 2, src, err);
	FREAD(&drop_amount, sizeof(drop_amount), 1, src, err);

	uint32_t n_species;
	FREAD(&n_species, sizeof(n_species), 1, src, err);
	n_species = ntohl(n_species);
	struct brain **species = calloc(n_species , sizeof(struct brain *));
	if (!species) {
		*err = "too many species";
		return NULL;
	}
	for (uint32_t i = 0; i < n_species; ++i) {
		struct brain *b = read_brain(src, err);
		if (!b)
			return NULL;
		species[i] = b;
	}
	uint32_t dims[2];
	FREAD(dims, sizeof(*dims), 2, src, err);
	struct grid *g = grid_new(ntohl(dims[0]), ntohl(dims[1])); /* TODO: Use a function with less
								      built-in initialization. */
	g->tick = ntohs(fields16[0]);
	g->drop_interval = ntohs(fields16[1]);
	g->health = ntohs(fields16[2]);
	g->lifetime = ntohs(fields16[3]);
	g->random = ntohl(fields32[0]);
	g->mutate_chance = ntohl(fields32[1]);
	g->drop_amount = drop_amount;

	long next_tile = ftell(src);
	if (next_tile == -1)
		return NULL;
	for (size_t i = 0; i < g->width * g->height; ++i) {
		uint32_t animal;
		FREAD(&animal, sizeof(animal), 1, src, err);
		FREAD(g->tiles[i].chemicals, sizeof(*g->tiles[i].chemicals), N_CHEMICALS, src, err);
		next_tile += STORED_TILE_SIZE;
		if (animal) {
			if (fseek(src, ntohl(animal) - STORED_TILE_SIZE, SEEK_CUR))
				FAIL_PTR(fseek, err);
			struct animal *a = read_animal(species, n_species, src, err);
			if (!a) {
				return NULL;
			}
			g->tiles[i].animal = a;
			if (fseek(src, next_tile, SEEK_SET))
				FAIL_PTR(fseek, err);
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

static int write_instruction(const struct instruction *i, FILE *dest, const char **err)
{
	uint8_t op[2];
	op[0] = i->opcode;
	op[1] = (i->l_fmt << 4) | i->r_fmt;
	FWRITE(op, sizeof(*op), 2, dest, err);
	uint16_t args[2] = {htons(i->left), htons(i->right)};
	FWRITE(args, sizeof(*args), 2, dest, err);
	return 0;
}

static int read_instruction(struct instruction *dest, FILE *src, const char **err)
{
	uint8_t op[2];
	if (fread(op, sizeof(*op), 2, src) != 2) {
		if (feof(src)) {
			errno = EPROTO;
			*err = "unexpected end of file";
		} else
			*err = "fread failed";
		return -1;
	}
	dest->opcode = op[0];
	dest->l_fmt = op[1] >> 4;
	dest->r_fmt = op[1] & 3;
	uint16_t args[2];
	if (fread(args, sizeof(*args), 2, src) != 2) {
		if (feof(src)) {
			errno = EPROTO;
			*err = "unexpected end of file";
		} else
			*err = "fread failed";
		return -1;
	}
	dest->left = ntohs(args[0]);
	dest->right = ntohs(args[1]);
	return 0;
}

static int write_brain(const struct brain *b, FILE *dest, const char **err)
{
	uint16_t header[3] = {htons(b->signature), htons(b->ram_size), htons(b->code_size)};
	FWRITE(header, sizeof(*header), 3, dest, err);
	for (uint16_t i = 0; i < b->code_size; ++i)
		if (write_instruction(&b->code[i], dest, err))
			return -1;
	return 0;
}

static int write_tile(const struct tile *t, uint32_t animal_off, FILE *dest, const char **err)
{
	animal_off = htonl(animal_off);
	FWRITE(&animal_off, sizeof(animal_off), 1, dest, err);
	FWRITE(t->chemicals, sizeof(*t->chemicals), N_CHEMICALS, dest, err);
	return 0;
}

static int write_animal(const struct animal *a, FILE *dest, const char **err)
{
	FWRITE(&a->brain->save_num, sizeof(a->brain->save_num), 1, dest, err);
	uint16_t fields16[5] = {
		htons(a->health), htons(a->energy), htons(a->lifetime),
		htons(a->instr_ptr), htons(a->flags)
	};
	FWRITE(fields16, sizeof(*fields16), 5, dest, err);
	FWRITE(a->stomach, sizeof(*a->stomach), N_CHEMICALS, dest, err);
	for (uint16_t i = 0; i < a->brain->ram_size; ++i) {
		uint16_t cell = htons(a->ram[i]);
		FWRITE(&cell, sizeof(cell), 1, dest, err);
	}
	return 0;
}

static struct brain *read_brain(FILE *src, const char **err)
{
	uint16_t fields16[3];
	FREAD(fields16, sizeof(*fields16), 3, src, err);
	struct brain *b = malloc(sizeof(struct brain) +
			ntohs(fields16[2]) * sizeof(struct instruction));
	b->signature = ntohs(fields16[0]);
	b->ram_size = ntohs(fields16[1]);
	b->code_size = ntohs(fields16[2]);
	for (uint16_t i = 0; i < b->code_size; ++i)
		if (read_instruction(&b->code[i], src, err))
			return NULL;
	return b;
}

static struct animal *read_animal(struct brain **species,
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
	struct animal *a = malloc(sizeof(struct animal) + b->ram_size * sizeof(uint16_t));
	a->brain = b;
	uint16_t fields16[5];
	FREAD(fields16, sizeof(*fields16), 5, src, err);
	a->health = ntohs(fields16[0]);
	a->energy = ntohs(fields16[1]);
	a->lifetime = ntohs(fields16[2]);
	a->instr_ptr = ntohs(fields16[3]);
	a->flags = ntohs(fields16[4]);
	FREAD(a->stomach, sizeof(*a->stomach), N_CHEMICALS, src, err);
	for (uint16_t i = 0; i < b->ram_size; ++i) {
		FREAD(&a->ram[i], sizeof(a->ram[i]), 1, src, err);
		a->ram[i] = ntohs(a->ram[i]);
	}
	return a;
}

/*
save version number: 4

number of species: 4
(species) {
	irrelevant data: TBD
	code size: 2
	code: code size * {
		opcode and format: 2
		left arg: 2
		right arg: 2
	}
}

width: 4
height: 4
(tiles) {
	animal offset: 4
	chemicals: N_CHEMICALS
} * width * height

(animals) {
	species number: 4
	irrelevant data: TBD
	RAM: RAM size * 2
}

 * */
