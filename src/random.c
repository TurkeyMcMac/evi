/*
 * The code for generating random noise.
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

