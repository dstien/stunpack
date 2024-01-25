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

#define STUNTS_SIZE_MAX          0xFFFFFF
#define STUNTS_SIZE_MIN          0x10
#define STUNTS_PASSES_MASK       0x7F
#define STUNTS_PASSES_RECUR      0x80

#define STUNTS_TYPE_RLE          0x01
#define STUNTS_TYPE_HUFF         0x02

int stunts_isValid(stpk_Context *ctx);
unsigned int stunts_decompress(stpk_Context *ctx);

// Peek at 24-bit data length.
inline unsigned int stunts_peekLength(unsigned char *data, unsigned int offset)
{
	return data[offset] | data[offset + 1] << 8 | data[offset + 2] << 16;
}

// Read 24-bit data length and advance buffer offset.
inline unsigned int stunts_readLength(stpk_Buffer *buf)
{
    unsigned int len = stunts_peekLength(buf->data, buf->offset);
	buf->offset += 3;
    return len;
}

#endif
