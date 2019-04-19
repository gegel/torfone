/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/*
 * ===================================================================
 *  TS 26.104
 *  REL-5 V5.4.0 2004-03
 *  REL-6 V6.1.0 2004-03
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

/*
 * interf_rom.h
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    Tables:           Subjective importance
 *                      Homing frames
 *
 *
 */

#pragma once

#ifndef _interf_rom_h_
#define _interf_rom_h_

/*
 * include files
 */
#include <stdint.h>

/*
 * definition of constants
 */

/* number of parameters */
#define PRMNO_MR475 17
#define PRMNO_MR515 19
#define PRMNO_MR59  19
#define PRMNO_MR67  19
#define PRMNO_MR74  19
#define PRMNO_MR795 23
#define PRMNO_MR102 39
#define PRMNO_MR122 57
#define PRMNO_MRDTX 5

/*
 * tables
 */
#ifndef IF2
#ifndef ETSI
static const uint8_t block_size[16] = { 13, 14, 16, 18, 20, 21, 27, 32,
	6, 0, 0, 0, 0, 0, 0, 1
};

static const uint8_t toc_byte[16] =
    { 0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C,
	0x44, 0x4C, 0x54, 0x5C, 0x64, 0x6C, 0x74, 0x7C
};
#endif
#else
/* One encoded frame (bytes) */
static const uint8_t block_size[16] = { 13, 14, 16, 18, 19, 21, 26, 31,
	5, 0, 0, 0, 0, 0, 0, 1
};
#endif

/* Homing frames for the decoder */
static const int16_t dhf_MR475[PRMNO_MR475] = {
	0x00F8,
	0x009D,
	0x001C,
	0x0066,
	0x0000,
	0x0003,
	0x0028,
	0x000F,
	0x0038,
	0x0001,
	0x000F,
	0x0031,
	0x0002,
	0x0008,
	0x000F,
	0x0026,
	0x0003
};

static const int16_t dhf_MR515[PRMNO_MR515] = {
	0x00F8,
	0x009D,
	0x001C,
	0x0066,
	0x0000,
	0x0003,
	0x0037,
	0x000F,
	0x0000,
	0x0003,
	0x0005,
	0x000F,
	0x0037,
	0x0003,
	0x0037,
	0x000F,
	0x0023,
	0x0003,
	0x001F
};

static const int16_t dhf_MR59[PRMNO_MR59] = {
	0x00F8,
	0x00E3,
	0x002F,
	0x00BD,
	0x0000,
	0x0003,
	0x0037,
	0x000F,
	0x0001,
	0x0003,
	0x000F,
	0x0060,
	0x00F9,
	0x0003,
	0x0037,
	0x000F,
	0x0000,
	0x0003,
	0x0037
};

static const int16_t dhf_MR67[PRMNO_MR67] = {
	0x00F8,
	0x00E3,
	0x002F,
	0x00BD,
	0x0002,
	0x0007,
	0x0000,
	0x000F,
	0x0098,
	0x0007,
	0x0061,
	0x0060,
	0x05C5,
	0x0007,
	0x0000,
	0x000F,
	0x0318,
	0x0007,
	0x0000
};

static const int16_t dhf_MR74[PRMNO_MR74] = {
	0x00F8,
	0x00E3,
	0x002F,
	0x00BD,
	0x0006,
	0x000F,
	0x0000,
	0x001B,
	0x0208,
	0x000F,
	0x0062,
	0x0060,
	0x1BA6,
	0x000F,
	0x0000,
	0x001B,
	0x0006,
	0x000F,
	0x0000
};

static const int16_t dhf_MR795[PRMNO_MR795] = {
	0x00C2,
	0x00E3,
	0x002F,
	0x00BD,
	0x0006,
	0x000F,
	0x000A,
	0x0000,
	0x0039,
	0x1C08,
	0x0007,
	0x000A,
	0x000B,
	0x0063,
	0x11A6,
	0x000F,
	0x0001,
	0x0000,
	0x0039,
	0x09A0,
	0x000F,
	0x0002,
	0x0001
};

static const int16_t dhf_MR102[PRMNO_MR102] = {
	0x00F8,
	0x00E3,
	0x002F,
	0x0045,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x001B,
	0x0000,
	0x0001,
	0x0000,
	0x0001,
	0x0326,
	0x00CE,
	0x007E,
	0x0051,
	0x0062,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x015A,
	0x0359,
	0x0076,
	0x0000,
	0x001B,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x017C,
	0x0215,
	0x0038,
	0x0030
};

static const int16_t dhf_MR122[PRMNO_MR122] = {
	0x0004,
	0x002A,
	0x00DB,
	0x0096,
	0x002A,
	0x0156,
	0x000B,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0036,
	0x000B,
	0x0000,
	0x000F,
	0x000E,
	0x000C,
	0x000D,
	0x0000,
	0x0001,
	0x0005,
	0x0007,
	0x0001,
	0x0008,
	0x0024,
	0x0000,
	0x0001,
	0x0000,
	0x0005,
	0x0006,
	0x0001,
	0x0002,
	0x0004,
	0x0007,
	0x0004,
	0x0002,
	0x0003,
	0x0036,
	0x000B,
	0x0000,
	0x0002,
	0x0004,
	0x0000,
	0x0003,
	0x0006,
	0x0001,
	0x0007,
	0x0006,
	0x0005,
	0x0000
};

/* parameter sizes (# of bits), one table per mode */
static const int16_t bitno_MR475[PRMNO_MR475] = {
	8, 8, 7,		/* LSP VQ          */
	8, 7, 2, 8,		/* first subframe  */
	4, 7, 2,		/* second subframe */
	4, 7, 2, 8,		/* third subframe  */
	4, 7, 2			/* fourth subframe */
};

static const int16_t bitno_MR515[PRMNO_MR515] = {
	8, 8, 7,		/* LSP VQ          */
	8, 7, 2, 6,		/* first subframe  */
	4, 7, 2, 6,		/* second subframe */
	4, 7, 2, 6,		/* third subframe  */
	4, 7, 2, 6		/* fourth subframe */
};

static const int16_t bitno_MR59[PRMNO_MR59] = {
	8, 9, 9,		/* LSP VQ          */
	8, 9, 2, 6,		/* first subframe  */
	4, 9, 2, 6,		/* second subframe */
	8, 9, 2, 6,		/* third subframe  */
	4, 9, 2, 6		/* fourth subframe */
};

static const int16_t bitno_MR67[PRMNO_MR67] = {
	8, 9, 9,		/* LSP VQ          */
	8, 11, 3, 7,		/* first subframe  */
	4, 11, 3, 7,		/* second subframe */
	8, 11, 3, 7,		/* third subframe  */
	4, 11, 3, 7		/* fourth subframe */
};

static const int16_t bitno_MR74[PRMNO_MR74] = {
	8, 9, 9,		/* LSP VQ          */
	8, 13, 4, 7,		/* first subframe  */
	5, 13, 4, 7,		/* second subframe */
	8, 13, 4, 7,		/* third subframe  */
	5, 13, 4, 7		/* fourth subframe */
};

static const int16_t bitno_MR795[PRMNO_MR795] = {
	9, 9, 9,		/* LSP VQ          */
	8, 13, 4, 4, 5,		/* first subframe  */
	6, 13, 4, 4, 5,		/* second subframe */
	8, 13, 4, 4, 5,		/* third subframe  */
	6, 13, 4, 4, 5		/* fourth subframe */
};

static const int16_t bitno_MR102[PRMNO_MR102] = {
	8, 9, 9,		/* LSP VQ          */
	8, 1, 1, 1, 1, 10, 10, 7, 7,	/* first subframe  */
	5, 1, 1, 1, 1, 10, 10, 7, 7,	/* second subframe */
	8, 1, 1, 1, 1, 10, 10, 7, 7,	/* third subframe  */
	5, 1, 1, 1, 1, 10, 10, 7, 7	/* fourth subframe */
};

static const int16_t bitno_MR122[PRMNO_MR122] = {
	7, 8, 9, 8, 6,		/* LSP VQ          */
	9, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5,	/* first subframe  */
	6, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5,	/* second subframe */
	9, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5,	/* third subframe  */
	6, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 5	/* fourth subframe */
};

static const int16_t bitno_MRDTX[PRMNO_MRDTX] = {
	3, 8, 9, 9, 6
};

#endif /* _interf_rom_h_ */

