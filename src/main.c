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

#define N_ANIMALS 1

void simulate_grid(struct grid *g, long ticks)
{
	printf("\x1B[2J");
	while (ticks--) {
		printf("\x1B[%luA", g->width);
		grid_draw(g, stdout);
		fflush(stdout);
		usleep(9000);
		grid_update(g);
	}
}

void save_grid(const char *file_name, long ticks)
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
	for (size_t i = 0; i < N_ANIMALS; ) {
		struct tile *t = grid_get_unck(g, rand() % g->width, rand() % g->height);
		if (!t->animal) {
			struct animal *a = animal_new(b, 10000);
			tile_set_animal(t, a);
			a->health = g->health;
			a->lifetime = g->lifetime;
			++i;
		}
	}
	simulate_grid(g, ticks);
	const char *err;
	if (write_grid(g, file, &err))
		printf("%s; %s.\n", strerror(errno), err);
	fclose(file);
	grid_free(g);
	exit(EXIT_SUCCESS);
}

void run_grid(const char *file_name, long ticks)
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
	simulate_grid(g, ticks);
	int status;
	freopen(file_name, "wb", file);
	if (write_grid(g, file, &err)) {
		printf("%s; %s.\n", strerror(errno), err);
		status = EXIT_FAILURE;
	} else {
		status = EXIT_SUCCESS;
	}
	grid_free(g);
	fclose(file);
	exit(status);
}

int main(int argc, char *argv[])
{
	char *buffer = malloc(800000);
	setbuffer(stdout, buffer, 800000);
	long ticks = strtol(argv[3], NULL, 10);
	switch (argv[1][0]) {
	case 'w':
		save_grid(argv[2], ticks);
		break;
	case 'r':
		run_grid(argv[2], ticks);
		break;
	default:
		break;
	}
	return EXIT_SUCCESS;
}
