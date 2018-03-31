#include "animal.h"
#include "chemicals.h"
#include "grid.h"
#include "save.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define DIRECT	0x0000
#define BABY	0x0001

static const struct instruction code[] = {
	{OP_PICK, 0,0, 4, (255 << 8) | CHEM_RED},
	{OP_PICK, 0,0, 4, (255 << 8) | CHEM_GREEN},
	{OP_PICK, 0,0, 4, (255 << 8) | CHEM_BLUE},
	{OP_CONV, 0,0, CHEM_GREEN, CHEM_BLUE}, /* Cyan */
	{OP_CONV, 0,0, CHEM_RED, CHEM_CYAN},   /* Energy */
	{OP_STEP, 1,0, DIRECT},
	{OP_JPNO, 0,0, 0x0000, FBLOCKED},

	{OP_EAT,  0,0, CHEM_ENERGY, 255},
	{OP_CONV, 0,0, CHEM_GREEN, CHEM_BLUE}, /* Cyan */
	{OP_CONV, 0,0, CHEM_BLUE, CHEM_RED}, /* Purple */
	{OP_CONV, 0,0, CHEM_CYAN, CHEM_PURPLE}, /* Codea */

	{OP_BABY, 1,0, DIRECT, 10000},
	{OP_INCR, 1,0, DIRECT},
	{OP_AND,  1,0, DIRECT, 3},

	{OP_JUMP, 0,0, 0x0000},
};

static const enum chemical spring_colors[] = {CHEM_RED, CHEM_GREEN, CHEM_BLUE};

#define array_len(arr) (sizeof((arr)) / sizeof(*(arr)))

#define N_ANIMALS 100

volatile sig_atomic_t running = 1;

void canceller(int _)
{
	(void)_;
	running = 0;
}

void simulate_grid(struct grid *g, long ticks)
{
//	printf("\x1B[2J");
	while (ticks--) {
/*		printf("\x1B[%luA", g->height);
		grid_draw(g, stdout);
		fflush(stdout);
		usleep(5000);
*/		grid_update(g);
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
	g->mutate_chance = UINT32_MAX / 20;
	g->health = 50;
	g->lifetime = 30000;
	g->drop_interval = 12;
	g->drop_amount = 140;
	g->random = rand();
	struct brain *b = brain_new(0xdead, 1, array_len(code));
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
	while (running) {
		simulate_grid(g, ticks);
		if (g->species != NULL) {
			freopen(file_name, "wb", file);
			if (write_grid(g, file, &err))
				fprintf(stderr, "%s; %s.\n", strerror(errno), err);
		} else {
			fprintf(stderr, "Extinct!\n");
			break;
		}
	}
	grid_print_species(g, 5, stderr);
	grid_free(g);
	fclose(file);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	struct sigaction cancel_handler;
	cancel_handler.sa_handler = canceller;
	sigaction(SIGINT, &cancel_handler, NULL);
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
