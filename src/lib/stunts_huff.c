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

#include "util.h"

#include "stunts_huff.h"

inline unsigned char stpk_getHuffByte(stpk_Context *ctx);

// Decompress Huffman coded sub-file.
unsigned int stunts_huff_decompress(stpk_Context *ctx)
{
	unsigned char levels, leafNodesPerLevel[STUNTS_HUFF_LEVELS_MAX], alphabet[STUNTS_HUFF_ALPH_LEN], symbols[STUNTS_HUFF_PREFIX_LEN], widths[STUNTS_HUFF_PREFIX_LEN];
	short codeOffsets[STUNTS_HUFF_LEVELS_MAX];
	unsigned short totalCodes[STUNTS_HUFF_LEVELS_MAX];
	unsigned int i, alphLen;
	int delta;

	levels = ctx->src.data[ctx->src.offset++];
	delta = UTIL_GET_FLAG(levels, STUNTS_HUFF_LEVELS_DELTA);
	levels &= STUNTS_HUFF_LEVELS_MASK;

	UTIL_VERBOSE1("  %-10s %d\n", "levels", levels);
	UTIL_VERBOSE1("  %-10s %d\n\n", "delta", delta);

	if (levels > STUNTS_HUFF_LEVELS_MAX) {
		UTIL_ERR("Huffman tree levels greater than %d, got %d\n", STUNTS_HUFF_LEVELS_MAX, levels);
		return 1;
	}

	for (i = 0; i < levels; i++) {
		leafNodesPerLevel[i] = ctx->src.data[ctx->src.offset++];
	}

	alphLen = stunts_huff_genOffsets(ctx, levels, leafNodesPerLevel, codeOffsets, totalCodes);

	if (alphLen > STUNTS_HUFF_ALPH_LEN) {
		UTIL_ERR("Alphabet longer than than %d, got %d\n", STUNTS_HUFF_ALPH_LEN, alphLen);
		return 1;
	}

	// Read alphabet.
	for (i = 0; i < alphLen; i++) alphabet[i] = ctx->src.data[ctx->src.offset++];
	UTIL_VERBOSE_ARR(alphabet, alphLen, "alphabet");

	if (ctx->src.offset > ctx->src.len) {
		UTIL_ERR("Reached end of source buffer while parsing Huffman header\n");
		return 1;
	}

	stunts_huff_genPrefix(ctx, levels, leafNodesPerLevel, alphabet, symbols, widths);

	return stunts_huff_decode(ctx, alphabet, symbols, widths, codeOffsets, totalCodes, delta);
}

// Generate offset table for translating Huffman codes wider than 8 bits to alphabet indices.
unsigned int stunts_huff_genOffsets(stpk_Context *ctx, unsigned int levels, const unsigned char *leafNodesPerLevel, short *codeOffsets, unsigned short *totalCodes)
{
	unsigned int level, codes = 0, alphLen = 0;

	for (level = 0; level < levels; level++) {
		codes *= 2;
		codeOffsets[level] = alphLen - codes;

		codes += leafNodesPerLevel[level];
		alphLen += leafNodesPerLevel[level];

		totalCodes[level] = codes;

		UTIL_VERBOSE1("  codeOffsets[%2d] = %6d  totalCodes[%2d] = %6d\n", level, codeOffsets[level], level, totalCodes[level]);
	}
	UTIL_VERBOSE1("\n");

	return alphLen;
}

// Generate prefix table for direct lookup of Huffman codes up to 8 bits wide.
void stunts_huff_genPrefix(stpk_Context *ctx, unsigned int levels, const unsigned char *leafNodesPerLevel, const unsigned char *alphabet, unsigned char *symbols, unsigned char *widths)
{
	unsigned int prefix, alphabetIndex, width = 1, maxWidth = UTIL_MIN(levels, STUNTS_HUFF_PREFIX_WIDTH);
	unsigned char leafNodes, totalNodes = STUNTS_HUFF_PREFIX_MSB, remainingNodes;

	// Fill all prefixes with data from last leaf node.
	for (prefix = 0, alphabetIndex = 0; width <= maxWidth; width++, totalNodes >>= 1) {
		for (leafNodes = leafNodesPerLevel[width - 1]; leafNodes > 0; leafNodes--, alphabetIndex++) {
			for (remainingNodes = totalNodes; remainingNodes; remainingNodes--, prefix++) {
				symbols[prefix] = alphabet[alphabetIndex];
				widths[prefix] = width;
			}
		}
	}
	UTIL_VERBOSE_ARR(symbols, prefix, "symbols");

	// Pad with escape value for codes wider than 8 bits.
	for (; prefix < STUNTS_HUFF_ALPH_LEN; prefix++) widths[prefix] = STUNTS_HUFF_WIDTH_ESC;
	UTIL_VERBOSE_ARR(widths, prefix, "widths");
}

// Decode Huffman codes.
unsigned int stunts_huff_decode(stpk_Context *ctx, const unsigned char *alphabet, const unsigned char *symbols, const unsigned char *widths, const short *codeOffsets, const unsigned short *totalCodes, int delta)
{
	unsigned char readWidth = 8, curWidth = 0, curByte, code, level, curOut = 0;
	unsigned short curWord = 0;
	unsigned int progress = 0;

	curWord = (stpk_getHuffByte(ctx) << 8) | stpk_getHuffByte(ctx);

	UTIL_NOVERBOSE("Huffman    [");

	UTIL_VERBOSE1("Decoding Huffman codes... \n");
	UTIL_VERBOSE2("\nsrcOff dstOff rW cW curWord               cd    Description\n");

	while (ctx->dst.offset < ctx->dst.len) {
		UTIL_VERBOSE2("~~~~~~ ~~~~~~ ~~ ~~ ~~~~~~~~~~~~~~~~~~~~~ ~~    ~~~~~~~~~~~~~~~~~~\n");

		code = (curWord & 0xFF00) >> 8;
		UTIL_VERBOSE_HUFF("Shifted %d bits", curWidth);

		curWidth = widths[code];

		// If code is wider than 8 bits, read more bits and decode with offset table.
		if (curWidth > STUNTS_HUFF_PREFIX_WIDTH) {
			if (curWidth != STUNTS_HUFF_WIDTH_ESC) {
				UTIL_ERR("Invalid escape value. curWidth != %02X, got %02X\n", STUNTS_HUFF_WIDTH_ESC, curWidth);
				return STPK_RET_ERR;
			}

			curByte = (curWord & 0x00FF);
			curWord >>= STUNTS_HUFF_PREFIX_WIDTH;
			UTIL_VERBOSE_HUFF("Escaping to offset table");

			// Read bit by bit until a level is found, starting at the max width of the prefix table.
			for (level = STUNTS_HUFF_PREFIX_WIDTH; 1; level++) {
				if (!readWidth) {
					curByte = stpk_getHuffByte(ctx);
					readWidth = 8;
					UTIL_VERBOSE_HUFF("Read %02X", ctx->src.data[ctx->src.offset - 1]);
				}

				curWord = (curWord << 1) + UTIL_GET_FLAG(curByte, STUNTS_HUFF_PREFIX_MSB);
				curByte <<= 1;
				readWidth--;
				UTIL_VERBOSE_HUFF("level = %d", level);

				if (level >= STUNTS_HUFF_LEVELS_MAX) {
					UTIL_ERR("Offset table out of bounds (%d >= %d)\n", level, STUNTS_HUFF_LEVELS_MAX);
					return STPK_RET_ERR;
				}

				if (curWord < totalCodes[level]) {
					curWord += codeOffsets[level];

					if (curWord > 0xFF) {
						UTIL_ERR("Alphabet index out of bounds (%04X > %04X)\n", curWord, STUNTS_HUFF_ALPH_LEN);
						return STPK_RET_ERR;
					}

					if (delta) {
						UTIL_VERBOSE_HUFF("Using symbol %02X as delta to previous output %02X", alphabet[curWord], curOut);
						curOut += alphabet[curWord];
					}
					else {
						curOut = alphabet[curWord];
					}

					ctx->dst.data[ctx->dst.offset++] = curOut;
					UTIL_VERBOSE_HUFF("Wrote %02X using offset table", curOut);

					break;
				}
			}

			// Read another byte since the processed code was wider than a byte.
			curWord = (curByte << readWidth) | stpk_getHuffByte(ctx);
			curWidth = 8 - readWidth;
			readWidth = 8;
			UTIL_VERBOSE_HUFF("Read %02X", ctx->src.data[ctx->src.offset - 1]);
		}
		// Code is 8 bits wide or less, do direct prefix lookup.
		else {
			if (delta) {
				UTIL_VERBOSE_HUFF("Using symbol %02X as delta to previous output %02X", symbols[code], curOut);
				curOut += symbols[code];
			}
			else {
				curOut = symbols[code];
			}
			ctx->dst.data[ctx->dst.offset++] = curOut;
			UTIL_VERBOSE_HUFF("Wrote %02X from prefix table", curOut);

			if (readWidth < curWidth) {
				curWord <<= readWidth;
				UTIL_VERBOSE_HUFF("Shifted %d bits", readWidth);

				curWidth -= readWidth;
				readWidth = 8;

				curWord |= stpk_getHuffByte(ctx);
				UTIL_VERBOSE_HUFF("Read %02X", ctx->src.data[ctx->src.offset - 1]);
			}
		}

		curWord <<= curWidth;
		readWidth -= curWidth;

		if ((ctx->src.offset - 1) > ctx->src.len && ctx->dst.offset < ctx->dst.len) {
			UTIL_ERR("Reached unexpected end of source buffer while decoding Huffman codes\n");
			return STPK_RET_ERR;
		}

		// Progress bar.
		if (ctx->verbosity && (ctx->verbosity < 3) && ((ctx->dst.offset * 100) / ctx->dst.len) >= (progress * 10)) {
			ctx->logCallback(STPK_LOG_INFO, "%4d%%", progress++ * 10);
		}
	}

	UTIL_NOVERBOSE("]\n");
	UTIL_VERBOSE1("\n");

	if (ctx->src.offset < ctx->src.len) {
		UTIL_WARN("Huffman decoding finished with unprocessed data left in source buffer (%d bytes left)\n", ctx->src.len - ctx->src.offset);
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
	if (ctx->format.stunts.version == STPK_FMT_STUNTS_VER_1_0) {
		byte = reverseBits[byte];
	}
	return byte;
}
