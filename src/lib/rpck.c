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

#include "rpck.h"

#include "util.h"

int rpck_isValid(stpk_Context *ctx)
{
    if (ctx->src.len < RPCK_SIZE_MIN) {
		return 0;
	}

    uint32_t finalLength = rpck_peekLength(ctx->src.data, 4);
    uint32_t savedLength = rpck_peekLength(ctx->src.data, 8);

    return rpck_checkMagic(ctx)
        && (finalLength - savedLength + RPCK_SIZE_MIN) == ctx->src.len;
}

unsigned int rpck_decompress(stpk_Context *ctx)
{
    if (ctx->src.len < RPCK_SIZE_MIN) {
        UTIL_ERR("Unexpected EOF while reading RPck header.\n");
		return 0;
	}
    if (!rpck_checkMagic(ctx)) {
        unsigned char magic[5];
        UTIL_ERR("Invalid magic bytes. Expected \"RPck\", got \"%s\"\n", magic, util_stringCharsSafe(ctx->src.data, magic, sizeof(magic)));
        return 0;
    }
    ctx->src.offset += 4;
    UTIL_VERBOSE1("Format: RPck\n");

    uint32_t finalLength = rpck_readLength(&ctx->src);
    UTIL_VERBOSE1("Final length  %d\n", finalLength);
    UTIL_VERBOSE1("Source length %d\n", ctx->src.len);

    uint32_t savedLength = rpck_readLength(&ctx->src);
    UTIL_VERBOSE1("Saved length  %d\n", savedLength);
    UTIL_VERBOSE1("Ratio %.2f\n", (float)finalLength / ctx->src.len);

    ctx->dst.len = finalLength;
    if (util_allocDst(ctx)) {
        return 1;
    }

    while (ctx->src.offset < ctx->src.len) {
        signed char ctrl = ctx->src.data[ctx->src.offset++];
        UTIL_VERBOSE2("Offset %04X  Read ctrl %d ", ctx->src.offset - 1, ctrl);
        if (ctrl < 0) {
            if (ctx->src.offset - ctrl > ctx->src.len) {
                UTIL_ERR("Attempted to read %d byte(s) past end of source buffer at offset %04X",
                    (ctx->src.offset - ctrl) - ctx->src.len,
                    ctx->src.offset);
                return 1;
            }
            if (ctx->dst.offset - ctrl > ctx->dst.len) {
                UTIL_ERR("Attempted to write %d byte(s) past end of destination buffer at offset %04X",
                    (ctx->dst.offset - ctrl) - ctx->dst.len,
                    ctx->dst.offset);
                return 1;
            }
            for (; ctrl; ctrl++) {
                UTIL_VERBOSE2(" %02X", ctx->src.data[ctx->src.offset]);
                ctx->dst.data[ctx->dst.offset++] = ctx->src.data[ctx->src.offset++];
            }
            UTIL_VERBOSE2("\n");
        }
        else {
            if (ctx->src.offset >= ctx->src.len) {
                UTIL_ERR("Attempted to read 1 byte past end of source buffer at offset %04X",
                    ctx->src.offset);
                return 1;
            }
            unsigned char data = ctx->src.data[ctx->src.offset++];
            if (ctx->dst.offset + ctrl + 1 > ctx->dst.len) {
                UTIL_ERR("Attempted to write %d byte(s) past end of destination buffer at offset %04X",
                    (ctx->dst.offset + ctrl + 1) - ctx->dst.len,
                    ctx->dst.offset);
                return 1;
            }
            UTIL_VERBOSE2(" x %02X\n", data);
            for (int i = ctrl + 1; i; i--) {
                ctx->dst.data[ctx->dst.offset++] = data;
            }
        }
    }

    return 0;
}
