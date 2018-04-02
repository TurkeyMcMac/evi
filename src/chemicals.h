/*
 * The interface for getting chemical information and combining chemicals.
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

#ifndef _CHEMICALS_H

#define _CHEMICALS_H

#include <stdint.h>

#define N_CHEMICALS 11
enum chemical {
	CHEM_SLUDGE,
	CHEM_RED,
	CHEM_GREEN,
	CHEM_BLUE,
	CHEM_YELLOW,
	CHEM_CYAN,
	CHEM_PURPLE,
	CHEM_CODEA,
	CHEM_CODEB,
	CHEM_ENERGY,
	CHEM_HEALTH,
};

extern const struct chemical_info {
	const char *name;
	uint16_t energy, health;
	uint16_t evaporation, flow;
} chemical_table[N_CHEMICALS];

/* Combinations
 * ------------
 * red    + green  = yellow
 * green  + blue   = cyan
 * blue   + red    = purple
 * cyan   + purple = codea
 * yellow + purple = codeb
 * green  + yellow = health
 * red    + cyan   = energy
 * default:          sludge
 * */
enum chemical combine_chemicals(enum chemical c1, enum chemical c2);

#endif /* Header guard */
