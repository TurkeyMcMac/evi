#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include "save.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define DIRECT	0x0000
#define BABY	0x0001

static const struct instruction code[] = {
	{OP_GNRG, 1,0, DIRECT},

	{OP_AND,  1,0, DIRECT, 3},

	{OP_PICK, 0,0, 4, (255 << 8) | CHEM_RED},
	{OP_PICK, 0,0, 4, (255 << 8) | CHEM_GREEN},
	{OP_PICK, 0,0, 4, (255 << 8) | CHEM_BLUE},
	{OP_CONV, 0,0, CHEM_GREEN, CHEM_BLUE}, /* Cyan */
	{OP_CONV, 0,0, CHEM_RED, CHEM_CYAN},   /* Energy */
	{OP_STEP, 1,0, DIRECT},
	{OP_JPNO, 0,0, 0x0002, FBLOCKED},

	{OP_EAT,  0,0, CHEM_ENERGY, 255},
	{OP_CONV, 0,0, CHEM_GREEN, CHEM_BLUE}, /* Cyan */
	{OP_CONV, 0,0, CHEM_BLUE, CHEM_RED}, /* Purple */
	{OP_CONV, 0,0, CHEM_CYAN, CHEM_PURPLE}, /* Codea */

	{OP_ADD,  1,0, DIRECT, 2},
	{OP_MOVE, 1,1, BABY, DIRECT},
	{OP_OR,   1,0, BABY, 1 << 14},
	{OP_AND,  1,0, DIRECT, 3},
	{OP_BABY, 1,1, DIRECT, BABY},
	{OP_DECR, 1,0, DIRECT},

	{OP_JUMP, 0,0, 0x0001},
};

static const enum chemical spring_colors[] = {CHEM_RED, CHEM_GREEN, CHEM_BLUE};

#define array_len(arr) (sizeof((arr)) / sizeof(*(arr)))

#define N_ANIMALS 100

void save_grid(const char *file_name)
{
	srand(time(NULL));
	FILE *file = fopen(file_name, "wb");
	if (!file) {
		printf("no such file\n");
		exit(EXIT_FAILURE);
	}
	struct grid *g = grid_new(50, 50);
	g->health = 50;
	g->lifetime = 55000;
	g->drop_interval = 10;
	g->drop_amount = 128;
	g->random = rand();
	struct brain *b = brain_new(0xdead, 2, array_len(code));
	memcpy(b->code, code, sizeof(code));
	b->next = g->species;
	g->species = b;
	size_t i;
	for (i = 0; i < N_ANIMALS; ++i) {
		struct animal *a = animal_new(b, 10000);
		struct tile *t = grid_get_unck(g, rand() % g->width, rand() % g->height);
		t->animal = a;
		grid_add_animal(g, a);
	}
	i = 1000;
	while (i--) {
		grid_draw(g, stdout);
		printf("\n");
		usleep(9000);
		grid_update(g);
	}
	const char *err;
	if (write_grid(g, file, &err))
		printf("%s; %s.\n", strerror(errno), err);
	fclose(file);
	grid_free(g);
	exit(EXIT_SUCCESS);
}

void run_grid(const char *file_name)
{
	FILE *file = fopen(file_name, "rb");
	if (!file) {
		printf("no such file\n");
		exit(EXIT_FAILURE);
	}
	const char *err;
	struct grid *g = read_grid(file, &err);
	if (!g) {
		printf("%s; %s.\n", strerror(errno), err);
		exit(EXIT_FAILURE);
	}
	size_t i = 1000;
	while (i--) {
		grid_draw(g, stdout);
		printf("\n");
		usleep(9000);
		grid_update(g);
	}
	grid_free(g);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	switch (argv[1][0]) {
	case 'w':
		save_grid(argv[2]);
		break;
	case 'r':
		run_grid(argv[2]);
		break;
	default:
		break;
	}
	return EXIT_SUCCESS;
}
