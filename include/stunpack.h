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

#ifndef STPK_STUNPACK_H
#define STPK_STUNPACK_H

#include <stddef.h>

#define STPK_VERSION "0.2.0"
#define STPK_NAME    "stunpack"
#define STPK_BUGS    "daniel@stien.org"

#define STPK_RET_OK                0
#define STPK_RET_ERR               1
#define STPK_RET_ERR_UNKNOWN_FMT   3
#define STPK_RET_ERR_DATA_LEFT    10

typedef enum {
	// Automatic format detection when decompressing.
	STPK_FMT_AUTO,
	// Custom compression format used by the PC versions of Stunts/4-D Sports Driving.
	STPK_FMT_STUNTS,
	// EA compression library by Frank Barchard used by 4-D Sports Driving for Amiga and 4-D Driving for PC98.
	STPK_FMT_BARCHARD,
	// Amiga RPck archiver format used for 3-d shapes in 4-D Sports Driving for Amiga.
	STPK_FMT_RPCK,
	STPK_FMT_UNKNOWN
} stpk_FmtType;

typedef enum {
	STPK_FMT_STUNTS_VER_AUTO,
	STPK_FMT_STUNTS_VER_1_0,
	STPK_FMT_STUNTS_VER_1_1
} stpk_FmtStuntsVer;

//typedef enum {
//	STPK_FMT_STUNTS_RLE,
//	STPK_FMT_STUNTS_HUFF
//} stpk_FmtStuntsMethod;

typedef struct {
	stpk_FmtStuntsVer version;
	int maxPasses;
} stpk_FmtStunts;

//typedef struct {
//} stpk_FmtBarchard;

//typedef struct {
//} stpk_FmtRpck;

typedef struct {
	stpk_FmtType type;
	union {
		stpk_FmtStunts   stunts;
		//stpk_FmtBarchard barchard;
		//stpk_FmtRpck     rpck;
	};
} stpk_Format;

typedef enum {
	STPK_LOG_INFO,
	STPK_LOG_WARN,
	STPK_LOG_ERR
} stpk_LogType;

typedef void (*stpk_LogCallback)(stpk_LogType type, const char *msg, ...);
typedef void* (*stpk_AllocCallback)(size_t size);
typedef void (*stpk_DeallocCallback)(void *ptr);

typedef struct {
	unsigned char *data;
	unsigned int  offset;
	unsigned int  len;
} stpk_Buffer;

typedef struct {
	stpk_Buffer          src;
	stpk_Buffer          dst;
	stpk_Format          format;
	int                  verbosity;
	stpk_LogCallback     logCallback;
	stpk_AllocCallback   allocCallback;
	stpk_DeallocCallback deallocCallback;
} stpk_Context;

stpk_Context stpk_init(stpk_Format format, int verbosity, stpk_LogCallback logCallback, stpk_AllocCallback allocCallback, stpk_DeallocCallback deallocCallback);
void stpk_deinit(stpk_Context *ctx);

unsigned int stpk_decompress(stpk_Context *ctx);

stpk_FmtType stpk_getFmtType(stpk_Context *ctx);

const char *stpk_fmtStuntsVerStr(stpk_FmtStuntsVer version);

#endif
