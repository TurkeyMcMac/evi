#include "chemicals.h"

const struct chemical_info chemical_table[N_CHEMICALS] = {
/*	 Name      Energy  Health  Evaporation  Flow */
	{"sludge",      0,      0,         128, 1024},
	{"red"   ,      2,      0,          64,    8},
	{"green" ,      2,      0,          64,    8},
	{"blue"  ,      2,      0,          64,    8},
	{"yellow",      2,      0,          32,   16},
	{"cyan"  ,      2,      0,          32,   16},
	{"purple",      2,      0,          32,   16},
	{"mgenrl",      0,      0,          16,    4},
	{"mbit"  ,      0,      0,          16,    4},
	{"marith",      0,      0,          16,    4},
	{"mcntrl",      0,      0,          16,    4},
	{"mspec" ,      0,      0,          16,    4},
	{"energy",     32,      0,           8,  256},
	{"health",      0,      2,           8,   32},
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
	case chemicals_id(CHEM_YELLOW, CHEM_CYAN):
		return CHEM_MGENRL;
	case chemicals_id(CHEM_CYAN, CHEM_PURPLE):
		return CHEM_MBIT;
	case chemicals_id(CHEM_YELLOW, CHEM_PURPLE):
		return CHEM_MARITH;
	case chemicals_id(CHEM_PURPLE, CHEM_RED):
		return CHEM_MCNTRL;
	case chemicals_id(CHEM_PURPLE, CHEM_GREEN):
		return CHEM_MSPEC;
	case chemicals_id(CHEM_YELLOW, CHEM_YELLOW):
		return CHEM_ENERGY;
	case chemicals_id(CHEM_CYAN, CHEM_CYAN):
		return CHEM_HEALTH;
	default:
		return CHEM_SLUDGE;
	}
}


