#ifndef _CHEMICALS_H

#define _CHEMICALS_H

#include <stdint.h>

#define N_CHEMICALS 14
enum chemical {
	CHEM_SLUDGE,
	CHEM_RED,
	CHEM_GREEN,
	CHEM_BLUE,
	CHEM_YELLOW,
	CHEM_CYAN,
	CHEM_PURPLE,
	CHEM_MGENRL,
	CHEM_MBIT,
	CHEM_MARITH,
	CHEM_MCNTRL,
	CHEM_MSPEC,
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
 * yellow + cyan   = mgenrl
 * cyan   + purple = mbit
 * yellow + purple = marith
 * purple + red    = mcntrl
 * purple + green  = mspec
 * yellow + yellow = energy
 * cyan   + cyan   = health
 * default:          sludge
 * */
enum chemical combine_chemicals(enum chemical c1, enum chemical c2);

#endif /* Header guard */
