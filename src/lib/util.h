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

#ifndef STPK_LIB_UTIL_H
#define STPK_LIB_UTIL_H

#include <stunpack.h>

#define UTIL_LOG(show, type, msg, ...) if (show && ctx->logCallback) ctx->logCallback((type), (msg), ## __VA_ARGS__)
#define UTIL_MSG(msg, ...)       UTIL_LOG(ctx->verbosity,      STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define UTIL_ERR(msg, ...)       UTIL_LOG(ctx->verbosity,      STPK_LOG_ERR,  (msg), ## __VA_ARGS__)
#define UTIL_WARN(msg, ...)      UTIL_LOG(ctx->verbosity,      STPK_LOG_WARN, (msg), ## __VA_ARGS__)
#define UTIL_NOVERBOSE(msg, ...) UTIL_LOG(ctx->verbosity == 1, STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define UTIL_VERBOSE1(msg, ...)  UTIL_LOG(ctx->verbosity >  1, STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define UTIL_VERBOSE2(msg, ...)  UTIL_LOG(ctx->verbosity >  2, STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define UTIL_VERBOSE_ARR(arr, len, name) if (ctx->verbosity > 1) util_printArray(ctx, arr, len, name)
#define UTIL_VERBOSE_HUFF(msg, ...) UTIL_VERBOSE2("%6d %6d %2d %2d %04X %s %02X -> " msg "\n", \
					ctx->src.offset, ctx->dst.offset, readWidth, curWidth, curWord, \
					util_stringBits16(curWord), code, ## __VA_ARGS__)

#define UTIL_GET_FLAG(data, mask) ((data & mask) == mask)
#define UTIL_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define UTIL_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

int util_allocDst(stpk_Context *ctx);
void util_dst2src(stpk_Context *ctx);

char *util_stringBits16(unsigned short val);
void util_printArray(const stpk_Context *ctx, const unsigned char *arr, unsigned int len, const char *name);

#endif
