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

/***************************************************************************
 *
 *	 File Name:  mathdp31.c
 *
 *	 Purpose:  Contains functions increased-precision arithmetic operations.
 *
 *		Below is a listing of all the functions in this file.  There
 *		is no interdependence among the functions.
 *
 *		L_mpy_ls()
 *
 ***************************************************************************/

#include "sc1200.h"
#include "mathhalf.h"

/****************************************************************************
 *
 *	   FUNCTION NAME: L_mpy_ls
 *
 *	   PURPOSE:    Multiply a 32 bit number (L_var2) and a 16 bit
 *				   number (var1) returning a 32 bit result. L_var2
 *				   is truncated to 31 bits prior to executing the
 *				   multiply.
 *
 *	   INPUTS:
 *
 *		 L_var2 			A Longword input variable
 *
 *		 var1				A Shortword input variable
 *
 *	   OUTPUTS: 			none
 *
 *	   RETURN VALUE:		A Longword value
 *
 *	   KEYWORDS: mult,mpy,multiplication
 *
 ***************************************************************************/

Longword L_mpy_ls(Longword L_var2, Shortword var1)
{
	Longword	L_varOut;
	Shortword	swtemp;


	swtemp = shr(extract_l(L_var2), 1);
	swtemp = (Shortword) ((short) 32767 & (short) swtemp);

	L_varOut = L_mult(var1, swtemp);
	L_varOut = L_shr(L_varOut, 15);
	L_varOut = L_mac(L_varOut, var1, extract_h(L_var2));
	return (L_varOut);
}

