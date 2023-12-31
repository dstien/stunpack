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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stunpack.h>

#define STPK_LOG(show, type, msg, ...) if (show && ctx->logCallback) ctx->logCallback((type), (msg), ## __VA_ARGS__)
#define STPK_MSG(msg, ...)       STPK_LOG(ctx->verbosity,      STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define STPK_ERR(msg, ...)       STPK_LOG(ctx->verbosity,      STPK_LOG_ERR,  (msg), ## __VA_ARGS__)
#define STPK_WARN(msg, ...)      STPK_LOG(ctx->verbosity,      STPK_LOG_WARN, (msg), ## __VA_ARGS__)
#define STPK_NOVERBOSE(msg, ...) STPK_LOG(ctx->verbosity == 1, STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define STPK_VERBOSE1(msg, ...)  STPK_LOG(ctx->verbosity >  1, STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define STPK_VERBOSE2(msg, ...)  STPK_LOG(ctx->verbosity >  2, STPK_LOG_INFO, (msg), ## __VA_ARGS__)
#define STPK_VERBOSE_ARR(arr, len, name) if (ctx->verbosity > 1) stpk_printArray(ctx, arr, len, name)
#define STPK_VERBOSE_HUFF(msg, ...) STPK_VERBOSE2("%6d %6d %2d %2d %04X %s %02X -> " msg "\n", \
					ctx->src.offset, ctx->dst.offset, readWidth, curWidth, curWord, \
					stpk_stringBits16(curWord), code, ## __VA_ARGS__)

#define STPK_GET_FLAG(data, mask) ((data & mask) == mask)
#define STPK_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

inline unsigned int stpk_rleCopyByte(stpk_Context *ctx, unsigned char cur, unsigned int rep);
inline unsigned char stpk_getHuffByte(stpk_Context *ctx);
inline void stpk_getLength(stpk_Buffer *buf, unsigned int *len);
inline void stpk_dst2src(stpk_Context *ctx);
char *stpk_stringBits16(unsigned short val);
void stpk_printArray(const stpk_Context *ctx, const unsigned char *arr, unsigned int len, const char *name);

const char *stpk_versionStr(stpk_Version version)
{
	switch (version) {
		case STPK_VER_AUTO:
			return "auto";
		case STPK_VER_STUNTS10:
			return "stunts1.0";
		case STPK_VER_STUNTS11:
			return "stunts1.1";
		default:
			return "Unknown";
	}
}

stpk_Context stpk_init(stpk_Version version, int maxPasses, int verbosity, stpk_LogCallback logCallback, stpk_AllocCallback allocCallback, stpk_DeallocCallback deallocCallback)
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
	ctx.version = version;
	ctx.maxPasses = maxPasses;
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

void inline stpk_dst2src(stpk_Context *ctx)
{
	if (ctx->src.data != NULL) {
		ctx->deallocCallback(ctx->src.data);
	}
	ctx->src.data = ctx->dst.data;
	ctx->src.len = ctx->dst.len;
	ctx->dst.data = NULL;
	ctx->src.offset = ctx->dst.offset = 0;
}

int inline stpk_allocDst(stpk_Context *ctx)
{
	if ((ctx->dst.data = (unsigned char*)ctx->allocCallback(sizeof(unsigned char) * ctx->dst.len)) == NULL) {
		STPK_ERR("Error allocating memory for destination buffer. (%s)\n", strerror(errno));
		return 1;
	}
	return 0;
}

int inline stpk_isRle(stpk_Buffer *buf)
{
	return buf->data[0] == STPK_TYPE_RLE && buf->data[7] == 0;
}

// Decompress sub-files in source buffer.
unsigned int stpk_decomp(stpk_Context *ctx)
{
	unsigned char passes, type, i;
	unsigned int retval = 1, finalLen, srcOffset;

	STPK_VERBOSE1("  %-10s %s\n", "version", stpk_versionStr(ctx->version));

	passes = ctx->src.data[ctx->src.offset];
	if (STPK_GET_FLAG(passes, STPK_PASSES_RECUR)) {
		ctx->src.offset++;

		passes &= STPK_PASSES_MASK;
		STPK_VERBOSE1("  %-10s %d\n", "passes", passes);

		stpk_getLength(&ctx->src, &finalLen);
		STPK_VERBOSE1("  %-10s %d\n", "finalLen", finalLen);
		STPK_VERBOSE1("    %-8s %d\n", "srcLen", ctx->src.len);
		STPK_VERBOSE1("    %-8s %.2f\n", "ratio", (float)finalLen / ctx->src.len);
	}
	else {
		passes = 1;
	}

	if (ctx->src.offset > ctx->src.len) {
		STPK_ERR("Reached EOF while parsing file header\n");
		return 1;
	}

	for (i = 0; i < passes; i++) {
		STPK_NOVERBOSE("Pass %d/%d: ", i + 1, passes);
		STPK_VERBOSE1("\nPass %d/%d\n", i + 1, passes);

		type = ctx->src.data[ctx->src.offset++];
		stpk_getLength(&ctx->src, &ctx->dst.len);
		STPK_VERBOSE1("  %-10s %d\n", "dstLen", ctx->dst.len);

		if (stpk_allocDst(ctx)) {
			return 1;
		}

		switch (type) {
			case STPK_TYPE_RLE:
				STPK_VERBOSE1("  %-10s Run-length encoding\n", "type");
				retval = stpk_decompRLE(ctx);
				break;
			case STPK_TYPE_HUFF:
				STPK_VERBOSE1("  %-10s Huffman coding\n", "type");
				srcOffset = ctx->src.offset;
				retval = stpk_decompHuff(ctx);
				// If selected version is "auto", check if we should retry with BB Stunts 1.0.
				if (ctx->version == STPK_VER_AUTO
					&& (
						// Decompression failed.
						retval == STPK_RET_ERR
						// Decompression had source data left, but it is the last pass.
						|| (retval == STPK_RET_ERR_DATA_LEFT && (i == (passes - 1)))
						// There are more passes, but the next is not valid RLE.
						|| ((i < (passes - 1)) && !stpk_isRle(&ctx->dst))
					)
				) {
					STPK_WARN("Huffman decompression with Stunts 1.1 bit stream format failed, retrying with Stunts 1.0 format.\n");
					ctx->version = STPK_VER_STUNTS10;
					ctx->src.offset = srcOffset;
					ctx->dst.offset = 0;
					STPK_NOVERBOSE("Pass %d/%d: ", i + 1, passes);
					retval = stpk_decompHuff(ctx);
					ctx->version = STPK_VER_AUTO;
				}

				// Data left must be checked for BB Stunts 1.0 bit stream detection
				// heuristics, but it is not an error. SDTITL.PVS in BB Stunts 1.1
				// has 95 bytes extra, which is random data that is ignored.
				if (retval == STPK_RET_ERR_DATA_LEFT) {
					retval = STPK_RET_OK;
				}
				break;
			default:
				STPK_ERR("Error parsing source file. Expected type 1 (run-length) or 2 (Huffman), got %02X\n", type);
				return 1;
		}

		if (retval) {
			return retval;
		}

		if (i + 1 == ctx->maxPasses && passes != ctx->maxPasses) {
			STPK_MSG("Parsing limited to %d decompression pass(es), aborting.\n", ctx->maxPasses);
			return 0;
		}

		// Destination buffer is source for next pass.
		if (i < (passes - 1)) {
			stpk_dst2src(ctx);
		}
	}

	return 0;
}

// Decompress run-length encoded sub-file.
unsigned int stpk_decompRLE(stpk_Context *ctx)
{
	unsigned int srcLen, dstLen, i;
	unsigned char unk, escLen, esc[STPK_RLE_ESCLEN_MAX], escLookup[STPK_RLE_ESCLOOKUP_LEN];

	stpk_getLength(&ctx->src, &srcLen);
	STPK_VERBOSE1("  %-10s %d\n", "srcLen", srcLen);

	unk = ctx->src.data[ctx->src.offset++];
	STPK_VERBOSE1("  %-10s %02X\n", "unk", unk);

	if (unk) {
		STPK_WARN("Unknown RLE header field (unk) is %02X, expected 0\n", unk);
	}

	escLen = ctx->src.data[ctx->src.offset++];
	STPK_VERBOSE1("  %-10s %d (no sequences = %d)\n\n", "escLen", escLen & STPK_RLE_ESCLEN_MASK, STPK_GET_FLAG(escLen, STPK_RLE_ESCLEN_NOSEQ));

	if ((escLen & STPK_RLE_ESCLEN_MASK) > STPK_RLE_ESCLEN_MAX) {
		STPK_ERR("escLen & STPK_RLE_ESCLEN_MASK greater than max length %02X, got %02X\n", STPK_RLE_ESCLEN_MAX, escLen & STPK_RLE_ESCLEN_MASK);
		return 1;
	}

	// Read escape codes.
	for (i = 0; i < (escLen & STPK_RLE_ESCLEN_MASK); i++) esc[i] = ctx->src.data[ctx->src.offset++];
	STPK_VERBOSE_ARR(esc, escLen & STPK_RLE_ESCLEN_MASK, "esc");

	if (ctx->src.offset > ctx->src.len) {
		STPK_ERR("Reached end of source buffer while parsing run-length header\n");
		return 1;
	}

	// Generate escape code lookup table where the index is the escape code
	// and the value is the escape code's positional property.
	for (i = 0; i < STPK_RLE_ESCLOOKUP_LEN; i++) escLookup[i] = 0;
	for (i = 0; i < (escLen & STPK_RLE_ESCLEN_MASK); i++) escLookup[esc[i]] = i + 1;
	STPK_VERBOSE_ARR(escLookup, STPK_RLE_ESCLOOKUP_LEN, "escLookup");

	STPK_NOVERBOSE("Run-length ");

	// Decode sequence run as a separate pass.
	if (!STPK_GET_FLAG(escLen, STPK_RLE_ESCLEN_NOSEQ)) {
		if (stpk_rleDecodeSeq(ctx, esc[STPK_RLE_ESCSEQ_POS])) {
			return 1;
		}

		srcLen = ctx->dst.offset;
		dstLen = ctx->dst.len;
		stpk_dst2src(ctx);
		ctx->src.len = srcLen;
		ctx->dst.len = dstLen;

		if (stpk_allocDst(ctx)) {
			return 1;
		}
	}

	return stpk_rleDecodeOne(ctx, escLookup);
}

// Decode sequence runs.
unsigned int stpk_rleDecodeSeq(stpk_Context *ctx, unsigned char esc)
{
	unsigned char cur;
	unsigned int progress = 0, seqOffset, rep, i;

	STPK_NOVERBOSE("[");

	STPK_VERBOSE1("Decoding sequence runs...    ");
	STPK_VERBOSE2("\n\nsrcOff dstOff rep seq\n");
	STPK_VERBOSE2("~~~~~~ ~~~~~~ ~~~ ~~~~~~~~\n");

	// We do not know the destination length for this pass, dst->len covers both RLE passes.
	while (ctx->src.offset < ctx->src.len) {
		cur = ctx->src.data[ctx->src.offset++];

		if (cur == esc) {
			seqOffset = ctx->src.offset;

			while ((cur = ctx->src.data[ctx->src.offset++]) != esc) {
				if (ctx->src.offset >= ctx->src.len) {
					STPK_ERR("Reached end of source buffer before finding sequence end escape code %02X\n", esc);
					return 1;
				}

				ctx->dst.data[ctx->dst.offset++] = cur;
			}

			rep = ctx->src.data[ctx->src.offset++] - 1; // Already wrote sequence once.
			STPK_VERBOSE2("%6d %6d %02X  %2.*X\n", ctx->src.offset, ctx->dst.offset, rep + 1, ctx->src.offset - seqOffset - 2, ctx->src.data[seqOffset]);

			while (rep--) {
				for (i = 0; i < (ctx->src.offset - seqOffset - 2); i++) {
					if (ctx->dst.offset >= ctx->dst.len) {
						STPK_ERR("Reached end of temporary buffer while writing repeated sequence\n");
						return 1;
					}

					ctx->dst.data[ctx->dst.offset++] = ctx->src.data[seqOffset + i];
				}
			}

		}
		else {
			ctx->dst.data[ctx->dst.offset++] = cur;
			STPK_VERBOSE2("%6d %6d     %02X\n", ctx->src.offset, ctx->dst.offset, cur);

			if (ctx->dst.offset > ctx->dst.len) {
				STPK_ERR("Reached end of temporary buffer while writing non-RLE byte\n");
				return 1;
			}
		}

		// Progress bar.
		if (ctx->verbosity && (ctx->verbosity < 3) && ((ctx->src.offset * 100) / ctx->src.len) >= (progress * 25)) {
			ctx->logCallback(STPK_LOG_INFO, "%4d%%", progress++ * 25);
		}
	}

	STPK_VERBOSE1("\n");
	STPK_NOVERBOSE("]   ");

	return 0;
}

// Decode single-byte runs.
unsigned int stpk_rleDecodeOne(stpk_Context *ctx, const unsigned char *escLookup)
{
	unsigned char cur;
	unsigned int progress = 0, rep;

	STPK_NOVERBOSE("[");

	STPK_VERBOSE1("Decoding single-byte runs... ");

	STPK_VERBOSE2("\n\nsrcOff dstOff   rep cur\n");
	STPK_VERBOSE2("~~~~~~ ~~~~~~ ~~~~~ ~~~\n");

	while (ctx->dst.offset < ctx->dst.len) {
		cur = ctx->src.data[ctx->src.offset++];

		if (ctx->src.offset > ctx->src.len) {
			STPK_ERR("Reached unexpected end of source buffer while decoding single-byte runs\n");
			return 1;
		}

		if (escLookup[cur]) {
			switch (escLookup[cur]) {
				// Type 1: One-byte counter for repetitions
				case 1:
					rep = ctx->src.data[ctx->src.offset];
					cur = ctx->src.data[ctx->src.offset + 1];
					ctx->src.offset += 2;

					if (stpk_rleCopyByte(ctx, cur, rep)) {
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

					if (stpk_rleCopyByte(ctx, cur, rep)) {
						return 1;
					}

					break;

				// Type n: n repetitions
				default:
					rep = escLookup[cur] - 1;
					cur = ctx->src.data[ctx->src.offset++];

					if (stpk_rleCopyByte(ctx, cur, rep)) {
						return 1;
					}
			}
		}
		else {
			ctx->dst.data[ctx->dst.offset++] = cur;
			STPK_VERBOSE2("%6d %6d        %02X\n", ctx->src.offset, ctx->dst.offset, cur);
		}

		// Progress bar.
		if (ctx->verbosity && (ctx->verbosity < 3) && ((ctx->src.offset * 100) / ctx->src.len) >= (progress * 25)) {
			ctx->logCallback(STPK_LOG_INFO, "%4d%%", progress++ * 25);
		}
	}

	STPK_VERBOSE1("\n");
	STPK_NOVERBOSE("]\n");

	if (ctx->src.offset < ctx->src.len) {
		STPK_WARN("RLE decoding finished with unprocessed data left in source buffer (%d bytes left)\n", ctx->src.len - ctx->src.offset);
	}

	return 0;
}

inline unsigned int stpk_rleCopyByte(stpk_Context *ctx, unsigned char cur, unsigned int rep)
{
	STPK_VERBOSE2("%6d %6d    %02X  %02X\n", ctx->src.offset, ctx->dst.offset, rep, cur);

	while (rep--) {
		if (ctx->dst.offset >= ctx->dst.len) {
			STPK_ERR("Reached end of temporary buffer while writing byte run\n");
			return 1;
		}

		ctx->dst.data[ctx->dst.offset++] = cur;
	}

	return 0;
}

// Decompress Huffman coded sub-file.
unsigned int stpk_decompHuff(stpk_Context *ctx)
{
	unsigned char levels, leafNodesPerLevel[STPK_HUFF_LEVELS_MAX], alphabet[STPK_HUFF_ALPH_LEN], symbols[STPK_HUFF_PREFIX_LEN], widths[STPK_HUFF_PREFIX_LEN];
	short codeOffsets[STPK_HUFF_LEVELS_MAX];
	unsigned short totalCodes[STPK_HUFF_LEVELS_MAX];
	unsigned int i, alphLen;
	int delta;

	levels = ctx->src.data[ctx->src.offset++];
	delta = STPK_GET_FLAG(levels, STPK_HUFF_LEVELS_DELTA);
	levels &= STPK_HUFF_LEVELS_MASK;

	STPK_VERBOSE1("  %-10s %d\n", "levels", levels);
	STPK_VERBOSE1("  %-10s %d\n\n", "delta", delta);

	if (levels > STPK_HUFF_LEVELS_MAX) {
		STPK_ERR("Huffman tree levels greater than %d, got %d\n", STPK_HUFF_LEVELS_MAX, levels);
		return 1;
	}

	for (i = 0; i < levels; i++) {
		leafNodesPerLevel[i] = ctx->src.data[ctx->src.offset++];
	}

	alphLen = stpk_huffGenOffsets(ctx, levels, leafNodesPerLevel, codeOffsets, totalCodes);

	if (alphLen > STPK_HUFF_ALPH_LEN) {
		STPK_ERR("Alphabet longer than than %d, got %d\n", STPK_HUFF_ALPH_LEN, alphLen);
		return 1;
	}

	// Read alphabet.
	for (i = 0; i < alphLen; i++) alphabet[i] = ctx->src.data[ctx->src.offset++];
	STPK_VERBOSE_ARR(alphabet, alphLen, "alphabet");

	if (ctx->src.offset > ctx->src.len) {
		STPK_ERR("Reached end of source buffer while parsing Huffman header\n");
		return 1;
	}

	stpk_huffGenPrefix(ctx, levels, leafNodesPerLevel, alphabet, symbols, widths);

	return stpk_huffDecode(ctx, alphabet, symbols, widths, codeOffsets, totalCodes, delta);
}

// Generate offset table for translating Huffman codes wider than 8 bits to alphabet indices.
unsigned int stpk_huffGenOffsets(stpk_Context *ctx, unsigned int levels, const unsigned char *leafNodesPerLevel, short *codeOffsets, unsigned short *totalCodes)
{
	unsigned int level, codes = 0, alphLen = 0;

	for (level = 0; level < levels; level++) {
		codes *= 2;
		codeOffsets[level] = alphLen - codes;

		codes += leafNodesPerLevel[level];
		alphLen += leafNodesPerLevel[level];

		totalCodes[level] = codes;

		STPK_VERBOSE1("  codeOffsets[%2d] = %6d  totalCodes[%2d] = %6d\n", level, codeOffsets[level], level, totalCodes[level]);
	}
	STPK_VERBOSE1("\n");

	return alphLen;
}

// Generate prefix table for direct lookup of Huffman codes up to 8 bits wide.
void stpk_huffGenPrefix(stpk_Context *ctx, unsigned int levels, const unsigned char *leafNodesPerLevel, const unsigned char *alphabet, unsigned char *symbols, unsigned char *widths)
{
	unsigned int prefix, alphabetIndex, width = 1, maxWidth = STPK_MIN(levels, STPK_HUFF_PREFIX_WIDTH);
	unsigned char leafNodes, totalNodes = STPK_HUFF_PREFIX_MSB, remainingNodes;

	// Fill all prefixes with data from last leaf node.
	for (prefix = 0, alphabetIndex = 0; width <= maxWidth; width++, totalNodes >>= 1) {
		for (leafNodes = leafNodesPerLevel[width - 1]; leafNodes > 0; leafNodes--, alphabetIndex++) {
			for (remainingNodes = totalNodes; remainingNodes; remainingNodes--, prefix++) {
				symbols[prefix] = alphabet[alphabetIndex];
				widths[prefix] = width;
			}
		}
	}
	STPK_VERBOSE_ARR(symbols, prefix, "symbols");

	// Pad with escape value for codes wider than 8 bits.
	for (; prefix < STPK_HUFF_ALPH_LEN; prefix++) widths[prefix] = STPK_HUFF_WIDTH_ESC;
	STPK_VERBOSE_ARR(widths, prefix, "widths");
}

// Decode Huffman codes.
unsigned int stpk_huffDecode(stpk_Context *ctx, const unsigned char *alphabet, const unsigned char *symbols, const unsigned char *widths, const short *codeOffsets, const unsigned short *totalCodes, int delta)
{
	unsigned char readWidth = 8, curWidth = 0, curByte, code, level, curOut = 0;
	unsigned short curWord = 0;
	unsigned int progress = 0;

	curWord = (stpk_getHuffByte(ctx) << 8) | stpk_getHuffByte(ctx);

	STPK_NOVERBOSE("Huffman    [");

	STPK_VERBOSE1("Decoding Huffman codes... \n");
	STPK_VERBOSE2("\nsrcOff dstOff rW cW curWord               cd    Description\n");

	while (ctx->dst.offset < ctx->dst.len) {
		STPK_VERBOSE2("~~~~~~ ~~~~~~ ~~ ~~ ~~~~~~~~~~~~~~~~~~~~~ ~~    ~~~~~~~~~~~~~~~~~~\n");

		code = (curWord & 0xFF00) >> 8;
		STPK_VERBOSE_HUFF("Shifted %d bits", curWidth);

		curWidth = widths[code];

		// If code is wider than 8 bits, read more bits and decode with offset table.
		if (curWidth > STPK_HUFF_PREFIX_WIDTH) {
			if (curWidth != STPK_HUFF_WIDTH_ESC) {
				STPK_ERR("Invalid escape value. curWidth != %02X, got %02X\n", STPK_HUFF_WIDTH_ESC, curWidth);
				return STPK_RET_ERR;
			}

			curByte = (curWord & 0x00FF);
			curWord >>= STPK_HUFF_PREFIX_WIDTH;
			STPK_VERBOSE_HUFF("Escaping to offset table");

			// Read bit by bit until a level is found, starting at the max width of the prefix table.
			for (level = STPK_HUFF_PREFIX_WIDTH; 1; level++) {
				if (!readWidth) {
					curByte = stpk_getHuffByte(ctx);
					readWidth = 8;
					STPK_VERBOSE_HUFF("Read %02X", ctx->src.data[ctx->src.offset - 1]);
				}

				curWord = (curWord << 1) + STPK_GET_FLAG(curByte, STPK_HUFF_PREFIX_MSB);
				curByte <<= 1;
				readWidth--;
				STPK_VERBOSE_HUFF("level = %d", level);

				if (level >= STPK_HUFF_LEVELS_MAX) {
					STPK_ERR("Offset table out of bounds (%d >= %d)\n", level, STPK_HUFF_LEVELS_MAX);
					return STPK_RET_ERR;
				}

				if (curWord < totalCodes[level]) {
					curWord += codeOffsets[level];

					if (curWord > 0xFF) {
						STPK_ERR("Alphabet index out of bounds (%04X > %04X)\n", curWord, STPK_HUFF_ALPH_LEN);
						return STPK_RET_ERR;
					}

					if (delta) {
						STPK_VERBOSE_HUFF("Using symbol %02X as delta to previous output %02X", alphabet[curWord], curOut);
						curOut += alphabet[curWord];
					}
					else {
						curOut = alphabet[curWord];
					}

					ctx->dst.data[ctx->dst.offset++] = curOut;
					STPK_VERBOSE_HUFF("Wrote %02X using offset table", curOut);

					break;
				}
			}

			// Read another byte since the processed code was wider than a byte.
			curWord = (curByte << readWidth) | stpk_getHuffByte(ctx);
			curWidth = 8 - readWidth;
			readWidth = 8;
			STPK_VERBOSE_HUFF("Read %02X", ctx->src.data[ctx->src.offset - 1]);
		}
		// Code is 8 bits wide or less, do direct prefix lookup.
		else {
			if (delta) {
				STPK_VERBOSE_HUFF("Using symbol %02X as delta to previous output %02X", symbols[code], curOut);
				curOut += symbols[code];
			}
			else {
				curOut = symbols[code];
			}
			ctx->dst.data[ctx->dst.offset++] = curOut;
			STPK_VERBOSE_HUFF("Wrote %02X from prefix table", curOut);

			if (readWidth < curWidth) {
				curWord <<= readWidth;
				STPK_VERBOSE_HUFF("Shifted %d bits", readWidth);

				curWidth -= readWidth;
				readWidth = 8;

				curWord |= stpk_getHuffByte(ctx);
				STPK_VERBOSE_HUFF("Read %02X", ctx->src.data[ctx->src.offset - 1]);
			}
		}

		curWord <<= curWidth;
		readWidth -= curWidth;

		if ((ctx->src.offset - 1) > ctx->src.len && ctx->dst.offset < ctx->dst.len) {
			STPK_ERR("Reached unexpected end of source buffer while decoding Huffman codes\n");
			return STPK_RET_ERR;
		}

		// Progress bar.
		if (ctx->verbosity && (ctx->verbosity < 3) && ((ctx->dst.offset * 100) / ctx->dst.len) >= (progress * 10)) {
			ctx->logCallback(STPK_LOG_INFO, "%4d%%", progress++ * 10);
		}
	}

	STPK_NOVERBOSE("]\n");
	STPK_VERBOSE1("\n");

	if (ctx->src.offset < ctx->src.len) {
		STPK_WARN("Huffman decoding finished with unprocessed data left in source buffer (%d bytes left)\n", ctx->src.len - ctx->src.offset);
		return STPK_RET_ERR_DATA_LEFT;
	}

	return STPK_RET_OK;
}

// Read a byte from the Huffman code bit stream, reverse bits if game version is Brøderbund Stunts 1.0.
inline unsigned char stpk_getHuffByte(stpk_Context *ctx)
{
	// https://graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
	static const unsigned char reverseBits[] = {
#		define R2(n)   (n),   (n + 2 * 64),   (n + 1 * 64),   (n + 3 * 64)
#		define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#		define R6(n) R4(n), R4(n + 2 *  4), R4(n + 1 *  4), R4(n + 3 *  4)
		R6(0), R6(2), R6(1), R6(3)
	};

	unsigned char byte = ctx->src.data[ctx->src.offset++];
	if (ctx->version == STPK_VER_STUNTS10) {
		byte = reverseBits[byte];
	}
	return byte;
}

// Read file length: WORD remainder + BYTE multiplier * 0x10000.
inline void stpk_getLength(stpk_Buffer *buf, unsigned int *len)
{
	*len =  buf->data[buf->offset] | buf->data[buf->offset + 1] << 8; // Read remainder.
	*len += 0x10000 * buf->data[buf->offset + 2]; // Add multiplier.
	buf->offset += 3;
}

// Write bit values as string to stpk_b16. Used in verbose output.
char *stpk_stringBits16(unsigned short val)
{
	static char stpk_b16[16 + 1];
	int i;
	for (i = 0; i < 16; i++) stpk_b16[(16 - 1) - i] = '0' + STPK_GET_FLAG(val, (1 << i));
	stpk_b16[i] = 0;

	return stpk_b16;
}

// Print formatted array. Used in verbose output.
void stpk_printArray(const stpk_Context *ctx, const unsigned char *arr, unsigned int len, const char *name)
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

