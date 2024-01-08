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

#include "stunts_huff.h"
#include "stunts_rle.h"
#include "util.h"

#include "stunts.h"

int inline stunts_isRle(stpk_Buffer *buf);

// Decompress sub-files in source buffer.
unsigned int stunts_decompress(stpk_Context *ctx)
{
	unsigned char passes, type, i;
	unsigned int retval = 1, finalLen, srcOffset;

	UTIL_VERBOSE1("  %-10s %s\n", "version", stpk_fmtStuntsVerStr(ctx->format.stunts.version));

	passes = ctx->src.data[ctx->src.offset];
	if (UTIL_GET_FLAG(passes, STUNTS_PASSES_RECUR)) {
		ctx->src.offset++;

		passes &= STUNTS_PASSES_MASK;
		UTIL_VERBOSE1("  %-10s %d\n", "passes", passes);

		stunts_getLength(&ctx->src, &finalLen);
		UTIL_VERBOSE1("  %-10s %d\n", "finalLen", finalLen);
		UTIL_VERBOSE1("    %-8s %d\n", "srcLen", ctx->src.len);
		UTIL_VERBOSE1("    %-8s %.2f\n", "ratio", (float)finalLen / ctx->src.len);
	}
	else {
		passes = 1;
	}

	if (ctx->src.offset > ctx->src.len) {
		UTIL_ERR("Reached EOF while parsing file header\n");
		return 1;
	}

	for (i = 0; i < passes; i++) {
		UTIL_NOVERBOSE("Pass %d/%d: ", i + 1, passes);
		UTIL_VERBOSE1("\nPass %d/%d\n", i + 1, passes);

		type = ctx->src.data[ctx->src.offset++];
		stunts_getLength(&ctx->src, &ctx->dst.len);
		UTIL_VERBOSE1("  %-10s %d\n", "dstLen", ctx->dst.len);

		if (util_allocDst(ctx)) {
			return 1;
		}

		switch (type) {
			case STUNTS_TYPE_RLE:
				UTIL_VERBOSE1("  %-10s Run-length encoding\n", "type");
				retval = stunts_rle_decompress(ctx);
				break;
			case STUNTS_TYPE_HUFF:
				UTIL_VERBOSE1("  %-10s Huffman coding\n", "type");
				srcOffset = ctx->src.offset;
				retval = stunts_huff_decompress(ctx);
				// If selected version is "auto", check if we should retry with BB Stunts 1.0.
				if (ctx->format.stunts.version == STPK_FMT_STUNTS_VER_AUTO
					&& (
						// Decompression failed.
						retval == STPK_RET_ERR
						// Decompression had source data left, but it is the last pass.
						|| (retval == STPK_RET_ERR_DATA_LEFT && (i == (passes - 1)))
						// There are more passes, but the next is not valid RLE.
						|| ((i < (passes - 1)) && !stunts_isRle(&ctx->dst))
					)
				) {
					UTIL_WARN("Huffman decompression with Stunts 1.1 bit stream format failed, retrying with Stunts 1.0 format.\n");
					ctx->format.stunts.version = STPK_FMT_STUNTS_VER_1_0;
					ctx->src.offset = srcOffset;
					ctx->dst.offset = 0;
					UTIL_NOVERBOSE("Pass %d/%d: ", i + 1, passes);
					retval = stunts_huff_decompress(ctx);
					ctx->format.stunts.version = STPK_FMT_STUNTS_VER_AUTO;
				}

				// Data left must be checked for BB Stunts 1.0 bit stream detection
				// heuristics, but it is not an error. SDTITL.PVS in BB Stunts 1.1
				// has 95 bytes extra, which is random data that is ignored.
				if (retval == STPK_RET_ERR_DATA_LEFT) {
					retval = STPK_RET_OK;
				}
				break;
			default:
				UTIL_ERR("Error parsing source file. Expected type 1 (run-length) or 2 (Huffman), got %02X\n", type);
				return 1;
		}

		if (retval) {
			return retval;
		}

		if (i + 1 == ctx->format.stunts.maxPasses && passes != ctx->format.stunts.maxPasses) {
			UTIL_MSG("Parsing limited to %d decompression pass(es), aborting.\n", ctx->format.stunts.maxPasses);
			return 0;
		}

		// Destination buffer is source for next pass.
		if (i < (passes - 1)) {
			util_dst2src(ctx);
		}
	}

	return 0;
}

int inline stunts_isRle(stpk_Buffer *buf)
{
	return buf->data[0] == STUNTS_TYPE_RLE && buf->data[7] == 0;
}
