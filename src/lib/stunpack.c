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

#include <stunpack.h>

#include "stunts.h"

stpk_Context stpk_init(stpk_Format format, int verbosity, stpk_LogCallback logCallback, stpk_AllocCallback allocCallback, stpk_DeallocCallback deallocCallback)
{
	stpk_Buffer empty = {
		.data = NULL,
		.offset = 0,
		.len = 0
	};

	// Open Watcom does not support designated initializers with struct values (2024-01-06)
	stpk_Context ctx;
	ctx.src = empty;
	ctx.dst = empty;
	ctx.format = format;
	ctx.verbosity = verbosity;
	ctx.logCallback = logCallback;
	ctx.allocCallback = allocCallback;
	ctx.deallocCallback = deallocCallback;

	return ctx;
}

void stpk_deinit(stpk_Context *ctx)
{
	if (ctx->deallocCallback) {
		if (ctx->src.data != NULL) {
			ctx->deallocCallback(ctx->src.data);
			ctx->src.data = NULL;
			ctx->src.len = 0;
			ctx->src.offset = 0;
		}
		if (ctx->dst.data != NULL) {
			ctx->deallocCallback(ctx->dst.data);
			ctx->dst.data = NULL;
			ctx->dst.len = 0;
			ctx->dst.offset = 0;
		}
	}
}

unsigned int stpk_decompress(stpk_Context *ctx)
{
	return stunts_decompress(ctx);
}

const char *stpk_fmtStuntsVerStr(stpk_FmtStuntsVer version)
{
	switch (version) {
		case STPK_FMT_STUNTS_VER_AUTO:
			return "auto";
		case STPK_FMT_STUNTS_VER_1_0:
			return "stunts1.0";
		case STPK_FMT_STUNTS_VER_1_1:
			return "stunts1.1";
		default:
			return "Unknown";
	}
}
