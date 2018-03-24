#include "save.h"

#include "animal.h"
#include "grid.h"
#include <arpa/inet.h>
#include <stdio.h>

#if N_CHEMICALS != 11
	#error "Be sure to change the version number when changing N_CHEMICALS!"
#endif
#define SERIALIZATION_VERSION 1

#define FAIL(fn, e) do { *(e) = #fn; return -1; } while (0)

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

	long n_species_off;
	FTELL(&n_species_off, dest, err);
	struct brain *b;
	uint32_t n_species = 0;
	SLLIST_FOR_EACH(g->species, b) {
		b->save_num = n_species++;
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
			FSEEK(dest, next_tile, SEEK_SET, err);
		} else {
			if (write_tile(t, 0, dest, err))
				return -1;
			next_tile += STORED_TILE_SIZE;
		}
	}
	return 0;
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
	FWRITE(&animal_off, sizeof(animal_off), 1, dest, err);
	FWRITE(t->chemicals, sizeof(*t->chemicals), N_CHEMICALS, dest, err);
	return 0;
}

static int write_animal(const struct animal *a, FILE *dest, const char **err)
{
	FWRITE(&a->brain->save_num, sizeof(a->brain->save_num), 1, dest, err);
	uint16_t short_fields[6] = {
		htons(a->health), htons(a->energy), htons(a->lifetime),
		htons(a->instr_ptr), htons(a->flags), htons(a->action)
	};
	FWRITE(short_fields, sizeof(*short_fields), 5, dest, err);
	FWRITE(a->stomach, sizeof(*a->stomach), N_CHEMICALS, dest, err);
	for (uint16_t i = 0; i < a->brain->ram_size; ++i) {
		uint16_t cell = htons(a->ram[i]);
		FWRITE(&cell, sizeof(cell), 1, dest, err);
	}
	return 0;
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
