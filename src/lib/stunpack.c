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

#include "dsi.h"
#include "rpck.h"
#include "util.h"

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
		case STPK_FMT_RPCK:
			return rpck_decompress(ctx);
		case STPK_FMT_DSI:
			return dsi_decompress(ctx);
		default:
			return STPK_RET_ERR_UNKNOWN_FMT;
	}
}

// Guess format type if user didn't specify format in context.
stpk_FmtType stpk_getFmtType(stpk_Context *ctx)
{
	if (ctx->format.type == STPK_FMT_AUTO) {
		if (rpck_isValid(ctx)) {
			ctx->format.type = STPK_FMT_RPCK;
		}
		// TODO: Check other header details, cleanup, move to eac.c.
		else if (ctx->src.data[1] == 0xFB) {
			ctx->format.type = STPK_FMT_EAC;
		}
		else if (dsi_isValid(ctx)) {
			ctx->format.type = STPK_FMT_DSI;
			ctx->format.dsi.version = STPK_FMT_DSI_VER_AUTO;
			ctx->format.dsi.maxPasses = 0;
		}
		else {
			ctx->format.type = STPK_FMT_UNKNOWN;
		}
	}

	return ctx->format.type;
}

const char *stpk_fmtTypeStr(stpk_FmtType type)
{
	switch (type) {
		case STPK_FMT_AUTO:
			return "auto";
		case STPK_FMT_DSI:
			return "dsi";
		case STPK_FMT_EAC:
			return "eac";
		case STPK_FMT_RPCK:
			return "rpck";
		default:
			return "unknown";
	}
}

const char *stpk_fmtDsiVerStr(stpk_FmtDsiVer version)
{
	switch (version) {
		case STPK_FMT_DSI_VER_AUTO:
			return "auto";
		case STPK_FMT_DSI_VER_1:
			return "dsi1";
		case STPK_FMT_DSI_VER_2:
			return "dsi2";
		default:
			return "unknown";
	}
}
