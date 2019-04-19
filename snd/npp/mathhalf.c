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

#include <stdlib.h>
#include <stdio.h>

#include "sc1200.h"
#include "constant.h"
#include "mathhalf.h"
#include "mathdp31.h"
#include "global.h"
#include "math.h"
#include "macro.h"

/***************************************************************************
 *
 *	 File Name:  mathhalf.c
 *
 *	 Purpose:  Contains functions which implement the primitive
 *	   arithmetic operations.
 *
 *		The functions in this file are listed below.  Some of them are
 *		defined in terms of other basic operations.  One of the
 *		routines, saturate() is static.  This is not a basic
 *		operation, and is not reference outside the scope of this
 *		file.
 *
 *
 *		 abs_s()
 *		 add()
 *		 divide_s()
 *		 extract_h()
 *		 extract_l()
 *		 L_abs()
 *		 L_add()
 *		 L_deposit_h()
 *		 L_deposit_l()
 *		 L_mac()
 *		 L_msu()
 *		 L_mult()
 *		 L_negate()
 *		 L_shift_r()
 *		 L_shl()
 *		 L_shr()
 *		 L_sub()
 *		 mac_r()
 *		 msu_r()
 *		 mult()
 *		 negate()
 *		 norm_l()
 *		 norm_s()
 *		 r_ound()
 *		 saturate()
 *		 shift_r()
 *		 shl()
 *		 shr()
 *		 sub()
 *
 **************************************************************************/

/***************************************************************************
 *
 *	 FUNCTION NAME: saturate
 *
 *	 PURPOSE:
 *
 *	   Limit the 32 bit input to the range of a 16 bit word.
 *
 *
 *	 INPUTS:
 *
 *	   L_var1
 *					   32 bit long signed integer (Longword) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   swOut
 *					   16 bit short signed integer (Shortword) whose value
 *					   falls in the range
 *					   0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *	 KEYWORDS: saturation, limiting, limit, saturate, 16 bits
 *
 *************************************************************************/

#if OVERFLOW_CHECK
inc_saturation()
{
	saturation++;
}
#else
 void inc_saturation()
{
}
#endif

