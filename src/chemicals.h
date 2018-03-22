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
 * red    + yellow = health
 * green  + cyan   = energy
 * default:          sludge
 * */
enum chemical combine_chemicals(enum chemical c1, enum chemical c2);

#endif /* Header guard */
