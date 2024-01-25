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
	// Custom compression format used by the MS DOS versions of many DSI games.
	STPK_FMT_DSI,
	// Custom compression format by EA Canada used by many EA games in the 90's and early 2000's.
	STPK_FMT_EAC,
	// Custom compression format used by the Amiga ports of some DSI games.
	STPK_FMT_RPCK,
	STPK_FMT_UNKNOWN
} stpk_FmtType;

typedef enum {
	// Attempt automatic detection by first trying version 2 and falling back
	// to version 1 if it fails.
	STPK_FMT_DSI_VER_AUTO,
	// Big-endian Huffman codes. Used by BB/MS Stunts 1.1.
	STPK_FMT_DSI_VER_1,
	// Little-endian Huffman codes. Used by BB Stunts 1.0 and earlier games.
	STPK_FMT_DSI_VER_2
} stpk_FmtDsiVer;

//typedef enum {
//	STPK_FMT_DSI_RLE  = 1,
//	STPK_FMT_DSI_HUFF = 2
//} stpk_FmtDsiMethod;

typedef struct {
	stpk_FmtDsiVer version;
	int maxPasses;
} stpk_FmtDsi;

//typedef struct {
//} stpk_FmtEac;

//typedef struct {
//} stpk_FmtRpck;

typedef struct {
	stpk_FmtType type;
	union {
		stpk_FmtDsi  dsi;
		//stpk_FmtEac  eac;
		//stpk_FmtRpck rpck;
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

const char *stpk_fmtTypeStr(stpk_FmtType type);
const char *stpk_fmtDsiVerStr(stpk_FmtDsiVer version);

#endif
