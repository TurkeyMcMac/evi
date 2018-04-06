/*
 * The code for writing to and reading from save files.
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

#include "brain.h"
#include "save.h"

#include <arpa/inet.h>
#include <stdlib.h>

#define RETURN_ERR (-1)
static int write_instruction(const struct instruction *i,
	FILE *dest,
	const char **err)
{
	uint8_t op[2];
	op[0] = i->opcode;
	op[1] = (i->l_fmt << 4) | i->r_fmt;
	FWRITE(op, sizeof(*op), 2, dest, err);
	uint16_t args[2] = {htons(i->left), htons(i->right)};
	FWRITE(args, sizeof(*args), 2, dest, err);
	return 0;
}

static int read_instruction(struct instruction *dest,
	FILE *src,
	const char **err)
{
	uint8_t op[2];
	FREAD(op, sizeof(*op), 2, src, err);
	dest->opcode = op[0];
	dest->l_fmt = op[1] >> 4;
	dest->r_fmt = op[1] & 3;
	uint16_t args[2];
	FREAD(args, sizeof(*args), 2, src, err);
	dest->left = ntohs(args[0]);
	dest->right = ntohs(args[1]);
	return 0;
}

int brain_write(const struct brain *b, FILE *dest, const char **err)
{
	uint16_t header[3] = {
		htons(b->signature), htons(b->ram_size), htons(b->code_size)
	};
	FWRITE(header, sizeof(*header), 3, dest, err);
	for (uint16_t i = 0; i < b->code_size; ++i) {
		if (write_instruction(&b->code[i], dest, err))
			return -1;
	}
	return 0;
}
#undef RETURN_ERR

#define RETURN_ERR NULL
struct brain *brain_read(FILE *src, const char **err)
{
	uint16_t fields16[3];
	FREAD(fields16, sizeof(*fields16), 3, src, err);
	struct brain *b = malloc(offsetof(struct brain, code) +
			ntohs(fields16[2]) * sizeof(struct instruction));
	b->signature = ntohs(fields16[0]);
	b->ram_size = ntohs(fields16[1]);
	b->code_size = ntohs(fields16[2]);
	for (uint16_t i = 0; i < b->code_size; ++i)
		if (read_instruction(&b->code[i], src, err))
			return NULL;
	return b;
}
