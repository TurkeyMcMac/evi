/*
 * The interface for writing to and reading from save files.
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

#ifndef _SAVE_H

#define _SAVE_H

#include "chemicals.h"
#include <errno.h>
#include <stdio.h>

#if N_CHEMICALS != 11
	#error "Be sure to change the version number when changing N_CHEMICALS!"
#endif
#define SERIALIZATION_VERSION 4

#define FAIL(fn, e) do { *(e) = #fn " failed"; return RETURN_ERR; } while (0)

#define FWRITE(src, size, nmemb, dest, e) do { \
	if (fwrite((src), (size), (nmemb), (dest)) != (nmemb)) \
		FAIL(fwrite, (e)); \
} while (0)

#define FTELL(dest, src, e) do { \
	*(dest) = ftell(src); \
	if (*(dest) == -1) \
		FAIL(ftell, (e)); \
} while (0)

#define FSEEK(dest, location, whence, e) do { \
	if (fseek((dest), (location), (whence))) \
		FAIL(fseek, (e)); \
} while (0)

#define FREAD(dest, size, nmemb, src, e) do { \
	if (fread((dest), (size), (nmemb), (src)) != (nmemb)) { \
		if (feof(src)) { \
			errno = EPROTO; \
			*(e) = "unexpected end of file"; \
			return RETURN_ERR; \
		} \
		FAIL(fread, (e)); \
	} \
} while (0)

#endif /* Header guard */
