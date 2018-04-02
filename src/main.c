/*
 * The main code to interface with the user.
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

void simulate_grid(struct grid *g, long ticks, char visual)
{
	if (visual == 'y') {
		printf("\x1B[2J");
		while (ticks--) {
			printf("\x1B[%luA", g->height);
			grid_draw(g, stdout);
			fflush(stdout);
			usleep(5000);
			grid_update(g);
		}
	} else
		while (ticks--)
			grid_update(g);
}

void save_grid(const char *file_name, long ticks, char visual)
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
	g->drop_interval = 17;
	g->drop_amount = 210;
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
			++i;
		}
	}
	simulate_grid(g, ticks, visual);
	const char *err;
	grid_print_species(g, 9, stdout);
	if (write_grid(g, file, &err))
		printf("%s; %s.\n", strerror(errno), err);
	fclose(file);
	grid_free(g);
	exit(EXIT_SUCCESS);
}

void run_grid(const char *file_name, long ticks, char visual)
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
		simulate_grid(g, ticks, visual);
		if (g->species != NULL) {
			freopen(file_name, "wb", file);
			if (write_grid(g, file, &err))
				fprintf(stderr, "%s; %s.\n", strerror(errno), err);
		} else {
			fprintf(stderr, "Extinct!\n");
			break;
		}
	}
	grid_print_species(g, 9, stderr);
	grid_free(g);
	fclose(file);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	struct sigaction cancel_handler;
	cancel_handler.sa_handler = canceller;
	sigaction(SIGINT, &cancel_handler, NULL);
	long ticks = strtol(argv[4], NULL, 10);
	switch (argv[1][0]) {
	case 'w':
		save_grid(argv[3], ticks, argv[2][0]);
		break;
	case 'r':
		run_grid(argv[3], ticks, argv[2][0]);
		break;
	default:
		break;
	}
	return EXIT_SUCCESS;
}
