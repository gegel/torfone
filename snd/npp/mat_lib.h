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

/* =================================================================== */
/* mat.h	 Matrix include file.                                      */
/*			 (Low level matrix and vector functions.)                  */
/*                                                                     */
/* Copyright (c) 1995 by Texas Instruments, Inc.  All rights reserved. */
/* =================================================================== */

#ifndef _MAT_LIB_H_
#define _MAT_LIB_H_


Shortword	*v_add(Shortword vec1[], Shortword vec2[], Shortword n);

Longword	*L_v_add(Longword L_vec1[], Longword L_vec2[], Shortword n);

Shortword	*v_equ(Shortword vec1[], Shortword v2[], Shortword n);

Shortword	*v_equ_shr(Shortword vec1[], Shortword vec2[], Shortword scale,
					   Shortword n);

Longword	*L_v_equ(Longword L_vec1[], Longword L_vec2[], Shortword n);

Shortword	v_inner(Shortword vec1[], Shortword vec2[], Shortword n,
					Shortword qvec1, Shortword qvec2, Shortword qout);

Longword	L_v_inner(Shortword vec1[], Shortword vec2[], Shortword n,
					  Shortword qvec1, Shortword qvec2, Shortword qout);

Shortword	v_magsq(Shortword vec1[], Shortword n, Shortword qvec1,
					Shortword qout);

Longword	L_v_magsq(Shortword vec1[], Shortword n, Shortword qvec1,
					  Shortword qout);

Shortword	*v_scale(Shortword vec1[], Shortword scale, Shortword n);

Shortword	*v_scale_shl(Shortword vec1[], Shortword scale, Shortword n,
						 Shortword shift);

Shortword	*v_sub(Shortword vec1[], Shortword vec2[], Shortword n);

Shortword	*v_zap(Shortword vec1[], Shortword n);

Longword	*L_v_zap(Longword L_vec1[], Shortword n);

Shortword	*v_get(Shortword n);

Longword	*L_v_get(Shortword n);

void	v_free(void *v);


#endif

