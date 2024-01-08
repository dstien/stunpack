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

#ifndef STPK_LIB_STUNTS_H
#define STPK_LIB_STUNTS_H

#include <stunpack.h>

#define STUNTS_MAX_SIZE          0xFFFFFF
#define STUNTS_PASSES_MASK       0x7F
#define STUNTS_PASSES_RECUR      0x80

#define STUNTS_TYPE_RLE          0x01
#define STUNTS_TYPE_HUFF         0x02

unsigned int stunts_decompress(stpk_Context *ctx);

// Read file length: WORD remainder + BYTE multiplier * 0x10000.
inline void stunts_getLength(stpk_Buffer *buf, unsigned int *len)
{
	*len =  buf->data[buf->offset] | buf->data[buf->offset + 1] << 8; // Read remainder.
	*len += 0x10000 * buf->data[buf->offset + 2]; // Add multiplier.
	buf->offset += 3;
}

#endif
