/*
 * The code for getting chemical information and combining chemicals.
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

#include "chemicals.h"

const struct chemical_info chemical_table[N_CHEMICALS] = {
/*	 Name      Energy  Health  Evaporation  Flow */
	{"sludge",      0,      0,         128,    5},
	{"red"   ,      2,      0,          99,    6},
	{"green" ,      2,      0,          98,    7},
	{"blue"  ,      2,      0,          97,    6},
	{"yellow",      2,      0,          98,    8},
	{"cyan"  ,      2,      0,          99,    9},
	{"purple",      2,      0,         101,   10},
	{"codea" ,	8,	0,	   120,	   4},
	{"codeb" ,	8,	0,	   120,	   4},
	{"energy",    512,      0,         159,   11},
	{"health",      0,      4,         160,   11},
};

#define chemicals_id(c1, c2) ((1 << (c1)) | (1 << (c2)))

enum chemical combine_chemicals(enum chemical c1, enum chemical c2)
{
	switch (chemicals_id(c1, c2)) {
	case chemicals_id(CHEM_RED, CHEM_GREEN):
		return CHEM_YELLOW;
	case chemicals_id(CHEM_GREEN, CHEM_BLUE):
		return CHEM_CYAN;
	case chemicals_id(CHEM_RED, CHEM_BLUE):
		return CHEM_PURPLE;
	case chemicals_id(CHEM_CYAN, CHEM_PURPLE):
		return CHEM_CODEA;
	case chemicals_id(CHEM_YELLOW, CHEM_PURPLE):
		return CHEM_CODEB;
	case chemicals_id(CHEM_RED, CHEM_CYAN):
		return CHEM_ENERGY;
	case chemicals_id(CHEM_GREEN, CHEM_YELLOW):
		return CHEM_HEALTH;
	default:
		return CHEM_SLUDGE;
	}
}
