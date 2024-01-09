/*
 * stunpack - Stunts/4D [Sports] Driving game resource unpacker
 * Copyright (C) 2008-2024 Daniel Stien <daniel@stien.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "util.h"

int util_allocDst(stpk_Context *ctx)
{
	if ((ctx->dst.data = (unsigned char*)ctx->allocCallback(sizeof(unsigned char) * ctx->dst.len)) == NULL) {
		UTIL_ERR("Error allocating memory for destination buffer. (%s)\n", strerror(errno));
		return 1;
	}
	return 0;
}

// Free old source buffer and set destination as new source for next pass.
void util_dst2src(stpk_Context *ctx)
{
	if (ctx->src.data != NULL) {
		ctx->deallocCallback(ctx->src.data);
	}
	ctx->src.data = ctx->dst.data;
	ctx->src.len = ctx->dst.len;
	ctx->dst.data = NULL;
	ctx->src.offset = ctx->dst.offset = 0;
}

 // Write bit values as string to static char[16] buffer. Used in verbose output.
char *util_stringBits16(unsigned short val)
{
	static char buf[16 + 1];
	int i;
	for (i = 0; i < 16; i++) buf[(16 - 1) - i] = '0' + UTIL_GET_FLAG(val, (1 << i));
	buf[i] = 0;

	return buf;
}

// Copy n-1 bytes from source to fixed destination buffer as printable ASCII characters.
unsigned char *util_stringCharsSafe(const unsigned char *src, unsigned char *dst, unsigned int len)
{
    for (unsigned i = 0; i < len - 1; i++) {
        dst[i] = isprint(src[i]) ? src[i] : '.';
    }
    dst[len] = 0;
    return dst;
}

// Print formatted array. Used in verbose output.
void util_printArray(const stpk_Context *ctx, const unsigned char *arr, unsigned int len, const char *name)
{
	unsigned int i = 0;

	ctx->logCallback(STPK_LOG_INFO, "  %s[%02X]\n", name, len);
	ctx->logCallback(STPK_LOG_INFO, "    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

	while (i < len) {
		if ((i % 0x10) == 0) ctx->logCallback(STPK_LOG_INFO, " %2X", i / 0x10);
		ctx->logCallback(STPK_LOG_INFO, " %02X", arr[i++]);
		if ((i % 0x10) == 0) ctx->logCallback(STPK_LOG_INFO, "\n");
	}

	if ((i % 0x10) != 0) ctx->logCallback(STPK_LOG_INFO, "\n");
	ctx->logCallback(STPK_LOG_INFO, "\n");
}
