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
	switch (stpk_getFmtType(ctx)) {
		case STPK_FMT_STUNTS:
			return stunts_decompress(ctx);
		default:
			return STPK_RET_ERR_UNKNOWN_FMT;
	}
}

// Guess format type if user didn't specify format in context.
stpk_FmtType stpk_getFmtType(stpk_Context *ctx)
{
	if (ctx->format.type == STPK_FMT_AUTO) {
		// TODO: Check other header details, cleanup, move to rpck.c.
		if (ctx->src.data[0] == 'R'
		&& ctx->src.data[1] == 'P'
		&& ctx->src.data[2] == 'c'
		&& ctx->src.data[3] == 'k') {
			ctx->format.type = STPK_FMT_RPCK;
		}
		// TODO: Check other header details, cleanup, move to barchard.c.
		else if (ctx->src.data[1] == 0xFB) {
			ctx->format.type = STPK_FMT_BARCHARD;
		}
		else if (stunts_isValid(ctx)) {
			ctx->format.type = STPK_FMT_STUNTS;
			ctx->format.stunts.version = STPK_FMT_STUNTS_VER_AUTO;
			ctx->format.stunts.maxPasses = 0;
		}
		else {
			ctx->format.type = STPK_FMT_UNKNOWN;
		}
	}

	return ctx->format.type;
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
