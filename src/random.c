#include "random.h"

static uint32_t rotright(uint32_t rot, uint32_t amount)
{
	amount %= 32;
	return (rot >> amount) | (rot << (32 - amount));
}

uint32_t randomize(uint32_t seed)
{
	return seed ^ rotright(seed ^ (seed * 31 - 1), seed);
}

