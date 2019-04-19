/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

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

/* ================================================= */
/* constant.h: include file for constant definitions */
/* ================================================= */

#ifndef _CONSTANT_H_
#define _CONSTANT_H_


#define ONE_Q9		512                                           /* (1 << 9) */
#define ONE_Q11		2048                                         /* (1 << 11) */
#define ONE_Q12		4096                                         /* (1 << 12) */
#define ONE_Q13		8192                                         /* (1 << 13) */
#define ONE_Q14		16384                                        /* (1 << 14) */
#define ONE_Q15		32767                                  /* ((1 << 15) - 1) */
#define ONE_Q19		524288L                                      /* (1 << 19) */
#define TWO_Q3		16                                        /* 2 * (1 << 3) */
#define TWO_Q19		1048576L                                 /* 2 * (1 << 19) */
#define THREE_Q11	6144                                     /* 3 * (1 << 11) */
#define FIFTY_Q0	50	                                    /* 50 * (1 << 0) */
#define FIFTY_Q7	6400                                     /* 50 * (1 << 7) */
#define SIX_Q8		1536                                      /* 6 * (1 << 8) */
#define X01_Q15		3277                                   /* 0.1 * (1 << 15) */
#define X02_Q15		6554                                   /* 0.2 * (1 << 15) */
#define X03_Q15		9830                                   /* 0.3 * (1 << 15) */
#define X008_Q15	2621                                  /* 0.08 * (1 << 15) */
#define X015_Q15	4915                                  /* 0.15 * (1 << 15) */
#define X05_Q6		32                                      /* 0.5 * (1 << 6) */
#define X05_Q14		8192                                   /* 0.5 * (1 << 14) */
#define X05_Q15		16384                                  /* 0.5 * (1 << 15) */
#define X07_Q15		22938                                  /* 0.7 * (1 << 15) */
#define X08_Q10		819                                    /* 0.8 * (1 << 10) */
#define X08_Q15		26214                                  /* 0.8 * (1 << 15) */
#define X09_Q15		29491                                  /* 0.9 * (1 << 15) */
#define X60_Q9		30720                                    /* 60 * (1 << 9) */
#define MONE_Q15	-32768                                    /* (-(1 << 15)) */

#define LW_SIGN		((long) 0x80000000)                           /* sign bit */
#define LW_MIN		((long) 0x80000000)
#define LW_MAX		((long) 0x7fffffff)

#define SW_MIN		((short) -32768)                  /* smallest RAM, 0x8000 */
#define SW_MAX		((short) 32767)                    /* largest RAM, 0x7fff */

#define MAX_40		((double)549755813883.0)
#define MIN_40		((double)-549755813884.0)

#define MAX_32		((Longword)0x7fffffffL)
#define MIN_32		((Longword)0x80000000L)

#define MAX_16		((Shortword)0x7fff)
#define MIN_16		((Shortword)0x8000)


#endif
