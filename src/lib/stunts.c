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

// Stunts compression does not have any identifier bytes, so we check if the
// contents corresponds to legal combinations of header values.
int stunts_isValid(stpk_Context *ctx)
{
	// Check if the source length is within the limits of what the format supports.
	if (ctx->src.len < STUNTS_SIZE_MIN || ctx->src.len > STUNTS_SIZE_MAX) {
		return 0;
	}

	unsigned int totalLength = stunts_peekLength(ctx->src.data, 2);

	// Check if total uncompressed length is larger than the source length.
	if (totalLength < UTIL_MAX(STUNTS_SIZE_MIN, ctx->src.len - STUNTS_SIZE_MIN)) {
		return 0;
	}

	// If the flag for multiple passes is set, a sane file will have
	// - 2 passes
	// - Total length longer than first pass' length
	// - First pass' length between SIZE_MIN and source length - SIZE_MIN
	// - First pass has either a valid RLE or Huffman header
	if (UTIL_GET_FLAG(ctx->src.data[0], STUNTS_PASSES_RECUR)) {
		unsigned char passes = ctx->src.data[0] & STUNTS_PASSES_MASK;
		unsigned int passLength = stunts_peekLength(ctx->src.data, 5);

		return passes == 2
			&& totalLength > passLength
			&& passLength > UTIL_MAX(STUNTS_SIZE_MIN, ctx->src.len - STUNTS_SIZE_MIN)
			&& (
				stunts_rle_isValid(&ctx->src, 4)
				|| stunts_huff_isValid(&ctx->src, 4)
			);
	}
	// A single pass file simply have a valid RLE of Huffman header
	else {
		return stunts_rle_isValid(&ctx->src, 0)
			|| stunts_huff_isValid(&ctx->src, 0);
	}
}

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

		finalLen = stunts_readLength(&ctx->src);
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
		ctx->dst.len = stunts_readLength(&ctx->src);
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
						|| ((i < (passes - 1)) && !stunts_rle_isValid(&ctx->dst, 0))
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
