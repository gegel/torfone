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
   math_lib.h	 Math include file.
				 (Log and Divide functions.)

   Copyright (c) 1997 by Texas Instruments, Inc.  All rights reserved.
*/

#ifndef _MATH_LIB_H_
#define _MATH_LIB_H_


/* External function definitions */

Shortword	L_divider2(Longword numer, Longword denom, Shortword numer_shift,
					   Shortword denom_shift);

Shortword	log10_fxp(Shortword x, Shortword Q);

Shortword	L_log10_fxp(Longword x, Shortword Q);

Shortword	pow10_fxp(Shortword x, Shortword Q);

Shortword	sqrt_fxp(Shortword x, Shortword Q);

Shortword	L_sqrt_fxp(Longword x, Shortword Q);

Shortword	L_pow_fxp(Longword x, Shortword power, Shortword Q_in,
					  Shortword Q_out);

Shortword	sin_fxp(Shortword x);

Shortword	cos_fxp(Shortword x);

Shortword	sqrt_Q15(Shortword x);

Shortword	add_shr(Shortword Var1, Shortword Var2);

Shortword	sub_shr(Shortword Var1, Shortword Var2);

#endif

