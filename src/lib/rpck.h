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

#ifndef STPK_LIB_RPCK_H
#define STPK_LIB_RPCK_H

#include <stdint.h>
#include <stunpack.h>

#define RPCK_SIZE_MIN 14

int rpck_isValid(stpk_Context *ctx);
unsigned int rpck_decompress(stpk_Context *ctx);

inline int rpck_checkMagic(stpk_Context *ctx)
{
    return ctx->src.data[0] == 'R'
		&& ctx->src.data[1] == 'P'
		&& ctx->src.data[2] == 'c'
		&& ctx->src.data[3] == 'k';
}

// Peek at 32-bit big endian data length.
inline uint32_t rpck_peekLength(unsigned char *data, unsigned offset)
{
    return data[offset + 0] << 24
        | data[offset + 1] << 16
        | data[offset + 2] << 8
        | data[offset + 3];
}

// Read 32-bit big endian data length and advance buffer offset.
inline uint32_t rpck_readLength(stpk_Buffer *buf)
{
    uint32_t len = rpck_peekLength(buf->data, buf->offset);
    buf->offset += 4;
    return len;
}

#endif
