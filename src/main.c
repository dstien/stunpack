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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__WATCOMC__)
#	include <unistd.h>
#	define stricmp strcasecmp
#else
#	include <getopt.h>
#endif

#include <stunpack.h>

#define BANNER STPK_NAME" "STPK_VERSION" - Stunts/4D [Sports] Driving game resource unpacker\n\n"
#define USAGE  "Usage: %s [OPTIONS]... SOURCE-FILE [DESTINATION-FILE]\n"

#define MSG(msg, ...) if (verbose) printf(msg, ## __VA_ARGS__)
#define ERR(msg, ...) if (verbose) fprintf(stderr, "\n" STPK_NAME ": " msg, ## __VA_ARGS__)
#define VERBOSE(msg, ...)  if (verbose > 1) printf(msg, ## __VA_ARGS__)

void printHelp(char *progName);
int decompress(char *srcFileName, char *dstFileName, stpk_Format format, int verbose);

int main(int argc, char **argv)
{
	char *srcFileName = NULL, *dstFileName = NULL;
	int retval = 0, opt, verbose = 1, srcFileNameLen = 0;
#if defined(__WATCOMC__)
	const int dstFileNamePostfixLen = 0;
#else
	const int dstFileNamePostfixLen = 4;
#endif
	stpk_FmtDsi dsi = {
		.version = STPK_FMT_DSI_VER_AUTO,
		.maxPasses = 0
	};
	stpk_Format format;
	//format.type = STPK_FMT_DSI;
	format.type = STPK_FMT_AUTO;
	//format.dsi = dsi;

	// Parse options.
	while ((opt = getopt(argc, argv, "f:s:p:hqv")) != -1) {
		switch (opt) {
			// Primary options
			case 'f':
				if (strcasecmp(optarg, stpk_fmtTypeStr(STPK_FMT_AUTO)) == 0) {
					format.type = STPK_FMT_AUTO;
				}
				else if (strcasecmp(optarg, stpk_fmtTypeStr(STPK_FMT_DSI)) == 0) {
					format.type = STPK_FMT_DSI;
					format.dsi = dsi;
				}
				else if (strcasecmp(optarg, stpk_fmtTypeStr(STPK_FMT_EAC)) == 0) {
					format.type = STPK_FMT_EAC;
				}
				else if (strcasecmp(optarg, stpk_fmtTypeStr(STPK_FMT_RPCK)) == 0) {
					format.type = STPK_FMT_RPCK;
				}
				else {
					fprintf(stderr, "Invalid format type \"%s\".\n", optarg);
					return 1;
				}
				break;

			// DSI format options
			case 's':
				if (format.type != STPK_FMT_DSI) {
					fprintf(stderr, "Format type must be \"%s\" for -s, got \"%s\"\n",
						stpk_fmtTypeStr(STPK_FMT_DSI),
						stpk_fmtTypeStr(format.type));
					return 1;
				}
				if (strcasecmp(optarg, stpk_fmtDsiVerStr(STPK_FMT_DSI_VER_AUTO)) == 0) {
					format.dsi.version = STPK_FMT_DSI_VER_AUTO;
				}
				else if (strcasecmp(optarg, stpk_fmtDsiVerStr(STPK_FMT_DSI_VER_1)) == 0) {
					format.dsi.version = STPK_FMT_DSI_VER_1;
				}
				else if (strcasecmp(optarg, stpk_fmtDsiVerStr(STPK_FMT_DSI_VER_2)) == 0) {
					format.dsi.version = STPK_FMT_DSI_VER_2;
				}
				else {
					fprintf(stderr, "Invalid DSI version \"%s\".\n", optarg);
					return 1;
				}
				break;	
			case 'p':
				if (format.type != STPK_FMT_DSI) {
					fprintf(stderr, "Format type must be \"%s\" for -p, got \"%s\"\n",
						stpk_fmtTypeStr(STPK_FMT_DSI),
						stpk_fmtTypeStr(format.type));
					return 1;
				}
				format.dsi.maxPasses = atoi(optarg);
				break;

			// General options
			case 'h':
				printHelp(argv[0]);
				return 0;
			case 'q':
				verbose = 0;
				break;
			case 'v':
				verbose++;
				break;
			case '?':
				retval = 1;
		}
	}

	if ((argc == optind) | (argc - optind > 2) | retval) {
		fprintf(stderr, USAGE, argv[0]);
		fprintf(stderr, "Try \"%s -h\" for help.\n", argv[0]);
		return 1;
	}

	MSG(BANNER);

	// Max two additional params (file names).
	for (; optind < argc; optind++) {
		if (srcFileName == NULL) {
			srcFileName = argv[optind];
		}
		else if (dstFileName == NULL) {
			dstFileName = argv[optind];
		}
	}

	// Generate destination file name if omitted.
	if (dstFileName == NULL) {
		srcFileNameLen = strlen(srcFileName);
		if ((dstFileName = (char*)malloc(sizeof(char) * (srcFileNameLen + dstFileNamePostfixLen + 1))) == NULL) {
			ERR("Error allocating memory for generated destination file name. (%s)\n", strerror(errno));
			return 1;
		}
		strcpy(dstFileName, srcFileName);
#if defined(__WATCOMC__)
		dstFileName[srcFileNameLen - 1] = '_';
#else
		strcat(dstFileName, ".out");
#endif
	}

	retval = decompress(srcFileName, dstFileName, format, verbose);

	// Clean up.
	if (dstFileName != NULL && srcFileNameLen) {
		free(dstFileName);
	}

	return retval;
}

void printHelp(char *progName)
{
	printf(BANNER);

	printf(USAGE, progName);

	printf("\n  Primary options\n");
	//printf("    -c       compress\n");
	//printf("    -d       decompress (default)\n");
	printf("    -f FMT   compression format: \"%s\" (default), \"%s\", \"%s\", \"%s\"\n\n",
		stpk_fmtTypeStr(STPK_FMT_AUTO),
		stpk_fmtTypeStr(STPK_FMT_DSI),
		stpk_fmtTypeStr(STPK_FMT_EAC),
		stpk_fmtTypeStr(STPK_FMT_RPCK));
	//printf("    -g GAME  set format and format options presets for given game:\n             \"%s\", \"%s\", \"%s\", \"%s\"\n\n",
	//	stpk_gameStr(STPK_GAME_STUNTS_1_0),
	//	stpk_gameStr(STPK_GAME_STUNTS_1_1),
	//	stpk_gameStr(STPK_GAME_4D_DRIVING_AMIGA),
	//	stpk_gameStr(STPK_GAME_4D_DRIVING_PC98)
	//);

	printf("  DSI format options\n");
	printf("    -s VER   format version: \"%s\" (default), \"%s\", \"%s\"\n",
		stpk_fmtDsiVerStr(STPK_FMT_DSI_VER_AUTO),
		stpk_fmtDsiVerStr(STPK_FMT_DSI_VER_1),
		stpk_fmtDsiVerStr(STPK_FMT_DSI_VER_2));
	printf("    -p NUM   limit to NUM decompression passes\n\n");

	printf("  General options\n");
	printf("    -v       verbose output\n");
	printf("    -vv      very verbose output\n");
	printf("    -q       no output\n");
	printf("    -h       print this text and exit\n\n");

	printf("Report bugs to <"STPK_BUGS">.\n");
}

void logCallback(stpk_LogType type, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	switch (type) {
		case STPK_LOG_WARN:
			fprintf(stderr, "Warning: ");
			vfprintf(stderr, msg, args);
			break;
		case STPK_LOG_ERR:
		fprintf(stderr, "Error: ");
			vfprintf(stderr, msg, args);
			break;
		case STPK_LOG_INFO:
		default:
			vprintf(msg, args);
			break;
	}

	va_end(args);
}

int decompress(char *srcFileName, char *dstFileName, stpk_Format format, int verbose)
{
	unsigned int retval = 1;
	FILE *srcFile, *dstFile;

	stpk_Context ctx = stpk_init(format, verbose, logCallback, malloc, free);

	if ((srcFile = fopen(srcFileName, "rb")) == NULL) {
		ERR("Error opening source file \"%s\" for reading. (%s)\n", srcFileName, strerror(errno));
		return 1;
	}

	MSG("Reading file \"%s\"...\n", srcFileName);

	if (fseek(srcFile, 0, SEEK_END) != 0) {
		ERR("Error seeking for EOF in source file \"%s\". (%s)\n", srcFileName, strerror(errno));
		goto closeSrcFile;
	}

	if ((ctx.src.len = ftell(srcFile)) == -1) {
		ERR("Error getting EOF position in source file \"%s\". (%s)\n", srcFileName, strerror(errno));
		goto closeSrcFile;
	}

	//if (ctx.src.len > STPK_MAX_SIZE) {
	//	ERR("Source file \"%s\" size (%d) exceeds max size (%d).\n", srcFileName, ctx.src.len, STPK_MAX_SIZE);
	//	goto closeSrcFile;
	//}

	if (fseek(srcFile, 0, SEEK_SET) != 0) {
		ERR("Error seeking for start position in source file \"%s\". (%s)\n", srcFileName, strerror(errno));
		goto closeSrcFile;
	}

	if ((ctx.src.data = (unsigned char*)malloc(sizeof(unsigned char) * ctx.src.len)) == NULL) {
		ERR("Error allocating memory for source file \"%s\" content. (%s)\n", srcFileName, strerror(errno));
		goto closeSrcFile;
	}

	if (fread(ctx.src.data, sizeof(unsigned char), ctx.src.len, srcFile) != ctx.src.len) {
		ERR("Error reading source file \"%s\" content. (%s)\n", srcFileName, strerror(errno));
		goto freeBuffers;
	}

	retval = stpk_decompress(&ctx);

	// Flush unpacked data to file.
	if (!retval) {
		VERBOSE("\n");
		MSG("Writing file \"%s\"... ", dstFileName);

		if ((dstFile = fopen(dstFileName, "wb")) == NULL) {
			ERR("Error opening destination file \"%s\" for writing. (%s)\n", dstFileName, strerror(errno));
			retval = 1;
			goto freeBuffers;
		}

		if (fwrite(ctx.dst.data, 1, ctx.dst.len, dstFile) != ctx.dst.len) {
			ERR("Error writing destination file \"%s\" content. (%s)\n", dstFileName, strerror(errno));
			retval = 1;
			goto closeDstFile;
		}

		MSG("Done!\n");

closeDstFile:
		if (fclose(dstFile) != 0) {
			ERR("Error closing destination file \"%s\". (%s)\n", dstFileName, strerror(errno));
			retval = 1;
		}
	}

freeBuffers:
	stpk_deinit(&ctx);

closeSrcFile:
	if (fclose(srcFile) != 0) {
		ERR("Error closing source file \"%s\". (%s)\n", srcFileName, strerror(errno));
		retval = 1;
	}

	return retval;
}

