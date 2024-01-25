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

#ifndef STPK_LIB_DSI_HUFF_H
#define STPK_LIB_DSI_HUFF_H

#include <stunpack.h>

#define DSI_HUFF_LEVELS_MASK  0x7F
#define DSI_HUFF_LEVELS_MAX   0x10
#define DSI_HUFF_LEVELS_DELTA 0x80

#define DSI_HUFF_ALPH_LEN     0x100
#define DSI_HUFF_PREFIX_WIDTH 0x08
#define DSI_HUFF_PREFIX_LEN   (1 << DSI_HUFF_PREFIX_WIDTH)
#define DSI_HUFF_PREFIX_MSB   (1 << (DSI_HUFF_PREFIX_WIDTH - 1))
#define DSI_HUFF_WIDTH_ESC    0x40

int dsi_huff_isValid(stpk_Buffer *buf, unsigned int offset);
unsigned int dsi_huff_decompress(stpk_Context *ctx);
unsigned int dsi_huff_genOffsets(stpk_Context *ctx, unsigned int levels, const unsigned char *leafNodesPerLevel, short *codeOffsets, unsigned short *totalCodes);
void dsi_huff_genPrefix(stpk_Context *ctx, unsigned int levels, const unsigned char *leafNodesPerLevel, const unsigned char *alphabet, unsigned char *symbols, unsigned char *widths);
unsigned int dsi_huff_decode(stpk_Context *ctx, const unsigned char *alphabet, const unsigned char *symbols, const unsigned char *widths, const short *codeOffsets, const unsigned short *totalCodes, int delta);

#endif
