/*

2.4 kbps MELP Proposed Federal Standard speech coder

Fixed-point C code, version 1.0

Copyright (c) 1998, Texas Instruments, Inc.

Texas Instruments has intellectual property rights on the MELP
algorithm.	The Texas Instruments contact for licensing issues for
commercial and non-government use is William Gordon, Director,
Government Contracts, Texas Instruments Incorporated, Semiconductor
Group (phone 972 480 7442).

The fixed-point version of the voice codec Mixed Excitation Linear
Prediction (MELP) is based on specifications on the C-language software
simulation contained in GSM 06.06 which is protected by copyright and
is the property of the European Telecommunications Standards Institute
(ETSI). This standard is available from the ETSI publication office
tel. +33 (0)4 92 94 42 58. ETSI has granted a license to United States
Department of Defense to use the C-language software simulation contained
in GSM 06.06 for the purposes of the development of a fixed-point
version of the voice codec Mixed Excitation Linear Prediction (MELP).
Requests for authorization to make other use of the GSM 06.06 or
otherwise distribute or modify them need to be addressed to the ETSI
Secretariat fax: +33 493 65 47 16.

*/

/*
   spbstd.h   SPB standard header file.

   Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved.
*/

#ifndef _MACRO_H_
#define _MACRO_H_


/* OSTYPE-dependent definitions/macros. */

#ifdef SunOS4

/* some standard C function definitions missing from SunOS4 */
extern int fclose(FILE *stream);
extern int fprintf(FILE *stream, const char *format, ...);
extern size_t fread(void *ptr, size_t size, size_t nobj, FILE *stream);
extern int fseek(FILE *stream, long offset, int origin);
extern size_t fwrite(const void *ptr, size_t size, size_t nobj, FILE *stream);
extern int printf(const char *format, ...);
extern long random(void);
extern int sscanf (char *s, const char *format, ...);
extern void rewind(FILE *stream);

#else

#endif

/* ===================== */
/* Constant definitions. */
/* ===================== */
#ifndef PI
#define PI				M_PI
#endif


/* ====== */
/* Macros */
/* ====== */

#define Max(a, b)		(((a) > (b)) ? (a) : (b))
#define Min(a, b)		(((a) > (b)) ? (b) : (a))

/* =============== */
/* Debugging Modes */
/* =============== */

void inc_saturation(void);
#if OVERFLOW_CHECK
#define save_saturation()		temp_saturation = saturation
#define restore_saturation()	saturation = temp_saturation
#else
#define save_saturation()
#define restore_saturation()
#endif

#endif

