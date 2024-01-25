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

#include "stunts.h"
#include "util.h"

#include "stunts_rle.h"

inline unsigned int stunts_rle_repeatByte(stpk_Context *ctx, unsigned char cur, unsigned int rep);

// Check if data at given offset is a likely RLE header:
// - Type is RLE
// - Reserved byte after length is 0x00
// - Escape code length between 1 and 10
int stunts_rle_isValid(stpk_Buffer *buf, unsigned int offset)
{
	return buf->data[offset + 0] == STUNTS_TYPE_RLE
		&& buf->data[offset + 7] == 0 // Reserved, always 0
        && (buf->data[offset + 8] & STUNTS_RLE_ESCLEN_MASK) >= 1
		&& (buf->data[offset + 8] & STUNTS_RLE_ESCLEN_MASK) <= STUNTS_RLE_ESCLEN_MAX;
}

// Decompress run-length encoded sub-file.
unsigned int stunts_rle_decompress(stpk_Context *ctx)
{
	unsigned int srcLen, dstLen, i;
	unsigned char unk, escLen, esc[STUNTS_RLE_ESCLEN_MAX], escLookup[STUNTS_RLE_ESCLOOKUP_LEN];

	srcLen = stunts_readLength(&ctx->src);
	UTIL_VERBOSE1("  %-10s %d\n", "srcLen", srcLen);

	unk = ctx->src.data[ctx->src.offset++];
	UTIL_VERBOSE1("  %-10s %02X\n", "unk", unk);

	if (unk) {
		UTIL_WARN("Unknown RLE header field (unk) is %02X, expected 0\n", unk);
	}

	escLen = ctx->src.data[ctx->src.offset++];
	UTIL_VERBOSE1("  %-10s %d (no sequences = %d)\n\n", "escLen", escLen & STUNTS_RLE_ESCLEN_MASK, UTIL_GET_FLAG(escLen, STUNTS_RLE_ESCLEN_NOSEQ));

	if ((escLen & STUNTS_RLE_ESCLEN_MASK) > STUNTS_RLE_ESCLEN_MAX) {
		UTIL_ERR("escLen & STUNTS_RLE_ESCLEN_MASK greater than max length %02X, got %02X\n", STUNTS_RLE_ESCLEN_MAX, escLen & STUNTS_RLE_ESCLEN_MASK);
		return 1;
	}

	// Read escape codes.
	for (i = 0; i < (escLen & STUNTS_RLE_ESCLEN_MASK); i++) esc[i] = ctx->src.data[ctx->src.offset++];
	UTIL_VERBOSE_ARR(esc, escLen & STUNTS_RLE_ESCLEN_MASK, "esc");

	if (ctx->src.offset > ctx->src.len) {
		UTIL_ERR("Reached end of source buffer while parsing run-length header\n");
		return 1;
	}

	// Generate escape code lookup table where the index is the escape code
	// and the value is the escape code's positional property.
	for (i = 0; i < STUNTS_RLE_ESCLOOKUP_LEN; i++) escLookup[i] = 0;
	for (i = 0; i < (escLen & STUNTS_RLE_ESCLEN_MASK); i++) escLookup[esc[i]] = i + 1;
	UTIL_VERBOSE_ARR(escLookup, STUNTS_RLE_ESCLOOKUP_LEN, "escLookup");

	UTIL_NOVERBOSE("Run-length ");

	// Decode sequence run as a separate pass.
	if (!UTIL_GET_FLAG(escLen, STUNTS_RLE_ESCLEN_NOSEQ)) {
		if (stunts_rle_decodeSeq(ctx, esc[STUNTS_RLE_ESCSEQ_POS])) {
			return 1;
		}

		srcLen = ctx->dst.offset;
		dstLen = ctx->dst.len;
		util_dst2src(ctx);
		ctx->src.len = srcLen;
		ctx->dst.len = dstLen;

		if (util_allocDst(ctx)) {
			return 1;
		}
	}

	return stunts_rle_decodeOne(ctx, escLookup);
}

// Decode sequence runs.
unsigned int stunts_rle_decodeSeq(stpk_Context *ctx, unsigned char esc)
{
	unsigned char cur;
	unsigned int progress = 0, seqOffset, rep, i;

	UTIL_NOVERBOSE("[");

	UTIL_VERBOSE1("Decoding sequence runs...    ");
	UTIL_VERBOSE2("\n\nsrcOff dstOff rep seq\n");
	UTIL_VERBOSE2("~~~~~~ ~~~~~~ ~~~ ~~~~~~~~\n");

	// We do not know the destination length for this pass, dst->len covers both RLE passes.
	while (ctx->src.offset < ctx->src.len) {
		cur = ctx->src.data[ctx->src.offset++];

		if (cur == esc) {
			seqOffset = ctx->src.offset;

			while ((cur = ctx->src.data[ctx->src.offset++]) != esc) {
				if (ctx->src.offset >= ctx->src.len) {
					UTIL_ERR("Reached end of source buffer before finding sequence end escape code %02X\n", esc);
					return 1;
				}

				ctx->dst.data[ctx->dst.offset++] = cur;
			}

			rep = ctx->src.data[ctx->src.offset++] - 1; // Already wrote sequence once.
			UTIL_VERBOSE2("%6d %6d %02X  %2.*X\n", ctx->src.offset, ctx->dst.offset, rep + 1, ctx->src.offset - seqOffset - 2, ctx->src.data[seqOffset]);

			while (rep--) {
				for (i = 0; i < (ctx->src.offset - seqOffset - 2); i++) {
					if (ctx->dst.offset >= ctx->dst.len) {
						UTIL_ERR("Reached end of temporary buffer while writing repeated sequence\n");
						return 1;
					}

					ctx->dst.data[ctx->dst.offset++] = ctx->src.data[seqOffset + i];
				}
			}

		}
		else {
			ctx->dst.data[ctx->dst.offset++] = cur;
			UTIL_VERBOSE2("%6d %6d     %02X\n", ctx->src.offset, ctx->dst.offset, cur);

			if (ctx->dst.offset > ctx->dst.len) {
				UTIL_ERR("Reached end of temporary buffer while writing non-RLE byte\n");
				return 1;
			}
		}

		// Progress bar.
		if (ctx->verbosity && (ctx->verbosity < 3) && ((ctx->src.offset * 100) / ctx->src.len) >= (progress * 25)) {
			ctx->logCallback(STPK_LOG_INFO, "%4d%%", progress++ * 25);
		}
	}

	UTIL_VERBOSE1("\n");
	UTIL_NOVERBOSE("]   ");

	return 0;
}

// Decode single-byte runs.
unsigned int stunts_rle_decodeOne(stpk_Context *ctx, const unsigned char *escLookup)
{
	unsigned char cur;
	unsigned int progress = 0, rep;

	UTIL_NOVERBOSE("[");

	UTIL_VERBOSE1("Decoding single-byte runs... ");

	UTIL_VERBOSE2("\n\nsrcOff dstOff   rep cur\n");
	UTIL_VERBOSE2("~~~~~~ ~~~~~~ ~~~~~ ~~~\n");

	while (ctx->dst.offset < ctx->dst.len) {
		cur = ctx->src.data[ctx->src.offset++];

		if (ctx->src.offset > ctx->src.len) {
			UTIL_ERR("Reached unexpected end of source buffer while decoding single-byte runs\n");
			return 1;
		}

		if (escLookup[cur]) {
			switch (escLookup[cur]) {
				// Type 1: One-byte counter for repetitions
				case 1:
					rep = ctx->src.data[ctx->src.offset];
					cur = ctx->src.data[ctx->src.offset + 1];
					ctx->src.offset += 2;

					if (stunts_rle_repeatByte(ctx, cur, rep)) {
						return 1;
					}

					break;

				// Type 2: Used for sequences. Serves no purpose here, but
				// would be handled by the default case if it were to occur.

				// Type 3: Two-byte counter for repetitions
				case 3:
					rep = ctx->src.data[ctx->src.offset] | ctx->src.data[ctx->src.offset + 1] << 8;
					cur = ctx->src.data[ctx->src.offset + 2];
					ctx->src.offset += 3;

					if (stunts_rle_repeatByte(ctx, cur, rep)) {
						return 1;
					}

					break;

				// Type n: n repetitions
				default:
					rep = escLookup[cur] - 1;
					cur = ctx->src.data[ctx->src.offset++];

					if (stunts_rle_repeatByte(ctx, cur, rep)) {
						return 1;
					}
			}
		}
		else {
			ctx->dst.data[ctx->dst.offset++] = cur;
			UTIL_VERBOSE2("%6d %6d        %02X\n", ctx->src.offset, ctx->dst.offset, cur);
		}

		// Progress bar.
		if (ctx->verbosity && (ctx->verbosity < 3) && ((ctx->src.offset * 100) / ctx->src.len) >= (progress * 25)) {
			ctx->logCallback(STPK_LOG_INFO, "%4d%%", progress++ * 25);
		}
	}

	UTIL_VERBOSE1("\n");
	UTIL_NOVERBOSE("]\n");

	if (ctx->src.offset < ctx->src.len) {
		UTIL_WARN("RLE decoding finished with unprocessed data left in source buffer (%d bytes left)\n", ctx->src.len - ctx->src.offset);
	}

	return 0;
}

inline unsigned int stunts_rle_repeatByte(stpk_Context *ctx, unsigned char cur, unsigned int rep)
{
	UTIL_VERBOSE2("%6d %6d    %02X  %02X\n", ctx->src.offset, ctx->dst.offset, rep, cur);

	while (rep--) {
		if (ctx->dst.offset >= ctx->dst.len) {
			UTIL_ERR("Reached end of temporary buffer while writing byte run\n");
			return 1;
		}

		ctx->dst.data[ctx->dst.offset++] = cur;
	}

	return 0;
}
