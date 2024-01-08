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

#ifndef STPK_LIB_STUNTS_RLE_H
#define STPK_LIB_STUNTS_RLE_H

#include <stunpack.h>

#define STUNTS_RLE_ESCLEN_MASK   0x7F
#define STUNTS_RLE_ESCLEN_MAX    0x0A
#define STUNTS_RLE_ESCLEN_NOSEQ  0x80
#define STUNTS_RLE_ESCLOOKUP_LEN 0x100
#define STUNTS_RLE_ESCSEQ_POS    0x01

unsigned int stunts_rle_decompress(stpk_Context *ctx);
unsigned int stunts_rle_decodeSeq(stpk_Context *ctx, unsigned char esc);
unsigned int stunts_rle_decodeOne(stpk_Context *ctx, const unsigned char *escLookup);

#endif
