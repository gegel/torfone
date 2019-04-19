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

/* mat_lib.c: Matrix and vector manipulation library                          */

#include <stdlib.h>
#include <assert.h>

#include "sc1200.h"
#include "mathhalf.h"
#include "mat_lib.h"

/***************************************************************************
 *
 *	 FUNCTION NAME: v_add
 *
 *	 PURPOSE:
 *
 *	   Perform the addition of the two 16 bit input vector with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (Shortword) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the addition of the two 16 bit input vectors with
 *	   saturation.
 *
 *	   vec1 = vec1 + vec2
 *
 *	   vec1[] is set to 0x7fff if the operation results in an
 *	   overflow.  vec1[] is set to 0x8000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: add, addition
 *
 *************************************************************************/

Shortword *v_add(Shortword vec1[], Shortword vec2[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = add(*vec1, *vec2);
		vec1 ++;
		vec2 ++;
	}
	return(vec1 - n);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_add
 *
 *	 PURPOSE:
 *
 *	   Perform the addition of the two 32 bit input vector with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   L_vec1		   32 bit long signed integer (Longword) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1 <= 0x7fff ffff.
 *
 *	   L_vec2		   32 bit long signed integer (Longword) vector whose
 *					   values falls in the range
 *					   0x8000 0000 <= L_vec2 <= 0x7fff ffff.
 *
 *	   n			   size of input vectors.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_vec1		   32 bit long signed integer (Longword) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1[] <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the addition of the two 32 bit input vectors with
 *	   saturation.
 *
 *	   L_vec1 = L_vec1 + L_vec2
 *
 *	   L_vec1[] is set to 0x7fff ffff if the operation results in an
 *	   overflow.  L_vec1[] is set to 0x8000 0000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: add, addition
 *
 *************************************************************************/

Longword *L_v_add(Longword L_vec1[], Longword L_vec2[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*L_vec1 = L_add(*L_vec1, *L_vec2);
		L_vec1 ++;
		L_vec2 ++;
	}
	return(L_vec1 - n);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: v_equ
 *
 *	 PURPOSE:
 *
 *	   Copy the contents of one 16 bit input vector to another
 *
 *	 INPUTS:
 *
 *	   vec2 		   16 bit short signed integer (Shortword) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vector
 *
 *	 OUTPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Copy the contents of one 16 bit input vector to another
 *
 *	   vec1 = vec2
 *
 *	 KEYWORDS: equate, copy
 *
 *************************************************************************/
Shortword *v_equ(Shortword vec1[], Shortword vec2[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = *vec2;
		vec1 ++;
		vec2 ++;
	}
	return(vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_equ_shr
 *
 *	 PURPOSE:
 *
 *	   Copy the contents of one 16 bit input vector to another with shift
 *
 *	 INPUTS:
 *
 *	   vec2 		   16 bit short signed integer (Shortword) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *	   scale		   right shift factor
 *	   n			   size of input vector
 *
 *	 OUTPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Copy the contents of one 16 bit input vector to another with shift
 *
 *	   vec1 = vec2>>scale
 *
 *	 KEYWORDS: equate, copy
 *
 *************************************************************************/

Shortword *v_equ_shr(Shortword vec1[], Shortword vec2[], Shortword scale,
					 Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = shr(*vec2, scale);
		vec1 ++;
		vec2 ++;
	}
	return(vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_equ
 *
 *	 PURPOSE:
 *
 *	   Copy the contents of one 32 bit input vector to another
 *
 *	 INPUTS:
 *
 *	   L_vec2		   32 bit long signed integer (Longword) vector whose
 *					   values falls in the range
 *					   0x8000 0000 <= L_vec2 <= 0x7fff ffff.
 *
 *	   n			   size of input vector
 *
 *	 OUTPUTS:
 *
 *	   L_vec1		   32 bit long signed integer (Longword) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1 <= 0x7fff ffff.
 *
 *	 RETURN VALUE:
 *
 *	   L_vec1		   32 bit long signed integer (Longword) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= L_vec1[] <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Copy the contents of one 32 bit input vector to another
 *
 *	   vec1 = vec2
 *
 *	 KEYWORDS: equate, copy
 *
 *************************************************************************/

Longword *L_v_equ(Longword L_vec1[], Longword L_vec2[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*L_vec1 = *L_vec2;
		L_vec1 ++;
		L_vec2 ++;
	}
	return(L_vec1 - n);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: v_inner
 *
 *	 PURPOSE:
 *
 *	   Compute the inner product of two 16 bit input vectors
 *	   with saturation and truncation.	Output is a 16 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qvec2		   Q value of vec2
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   innerprod	   16 bit short signed integer (Shortword) whose value
 *					   falls in the range
 *					   0xffff 8000 <= innerprod <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the inner product of the two 16 bit input vectors.
 *	   The output is a 16 bit number.
 *
 *	 KEYWORDS: inner product
 *
 *************************************************************************/

Shortword v_inner(Shortword vec1[], Shortword vec2[], Shortword n,
				  Shortword qvec1, Shortword qvec2, Shortword qout)
{
	register Shortword	i;
	Shortword	innerprod;
	Longword	L_temp;


	L_temp = 0;
	for (i = 0; i < n; i++){
		L_temp = L_mac(L_temp, *vec1, *vec2);
		vec1 ++;
		vec2 ++;
	}

	/* (qvec1 + qvec2 + 1) is the Q value from L_mult(vec1[i], vec2[i]), and  */
	/* also that for L_temp.  To make it Q qout, L_shl() it by                */
	/* (qout - (qvec1 + qvec2 + 1)).  To return only a Shortword, use         */
	/* extract_h() after L_shl() by 16.                                       */

	innerprod = extract_h(L_shl(L_temp,
								(Shortword) (qout - ((qvec1 + qvec2 + 1) -
											 16))));
	return(innerprod);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_inner
 *
 *	 PURPOSE:
 *
 *	   Compute the inner product of two 16 bit input vectors
 *	   with saturation and truncation.	Output is a 32 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qvec2		   Q value of vec2
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_innerprod	   32 bit long signed integer (Longword) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_innerprod <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the inner product of the two 16 bit vectors
 *	   The output is a 32 bit number.
 *
 *	 KEYWORDS: inner product
 *
 *************************************************************************/

Longword L_v_inner(Shortword vec1[], Shortword vec2[], Shortword n,
				   Shortword qvec1, Shortword qvec2, Shortword qout)
{
	register Shortword	i;
	Shortword	shift;
	Longword	L_innerprod, L_temp;


	L_temp = 0;
	for (i = 0; i < n; i++){
		L_temp = L_mac(L_temp, *vec1, *vec2);
		vec1 ++;
		vec2 ++;
	}

	/* L_temp is now (qvec1 + qvec2 + 1) */
	shift = sub(qout, add(add(qvec1, qvec2), 1));
	L_innerprod = L_shl(L_temp, shift);
	return(L_innerprod);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: v_magsq
 *
 *	 PURPOSE:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector
 *	   with saturation and truncation.	Output is a 16 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   magsq		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range
 *					   0xffff 8000 <= magsq <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector.
 *	   The output is a 16 bit number.
 *
 *	 KEYWORDS: square magnitude
 *
 *************************************************************************/

Shortword v_magsq(Shortword vec1[], Shortword n, Shortword qvec1,
				  Shortword qout)
{
	register Shortword	i;
	Shortword	shift;
	Shortword	magsq;
	Longword	L_temp;


	L_temp = 0;
	for (i = 0; i < n; i++){
		L_temp = L_mac(L_temp, *vec1, *vec1);
		vec1 ++;
	}
	/* qout - ((2*qvec1 + 1) - 16) */
	shift = sub(qout, sub(add(shl(qvec1, 1), 1), 16));
	magsq = extract_h(L_shl(L_temp, shift));
	return(magsq);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_magsq
 *
 *	 PURPOSE:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector
 *	   with saturation and truncation.	Output is a 32 bit number.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors
 *
 *	   qvec1		   Q value of vec1
 *
 *	   qout 		   Q value of output
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_magsq		   32 bit long signed integer (Longword) whose value
 *					   falls in the range
 *					   0x8000 0000 <= L_magsq <= 0x7fff ffff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Compute the sum of square magnitude of a 16 bit input vector.
 *	   The output is a 32 bit number.
 *
 *	 KEYWORDS: square magnitude
 *
 *************************************************************************/

Longword L_v_magsq(Shortword vec1[], Shortword n, Shortword qvec1,
				   Shortword qout)
{
	register Shortword	i;
	Shortword	shift;
	Longword	L_magsq, L_temp;


	L_temp = 0;
	for (i = 0; i < n; i++){
		L_temp = L_mac(L_temp, *vec1, *vec1);
		vec1 ++;
	}
	/* ((qout-16)-((2*qvec1+1)-16)) */
	shift = sub(sub(qout, shl(qvec1, 1)), 1);
	L_magsq = L_shl(L_temp, shift);
	return(L_magsq);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: v_scale
 *
 *	 PURPOSE:
 *
 *	   Perform a multipy of the 16 bit input vector with a 16 bit input
 *	   scale with saturation and truncation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *	   scale
 *					   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	   n			   size of vec1
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform a multipy of the 16 bit input vector with the 16 bit input
 *	   scale.  The output is a 16 bit vector.
 *
 *	 KEYWORDS: scale
 *
 *************************************************************************/

Shortword *v_scale(Shortword vec1[], Shortword scale, Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = mult(*vec1, scale);
		vec1 ++;
	}
	return(vec1 - n);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: v_scale_shl
 *
 *	 PURPOSE:
 *
 *	   Perform a multipy of the 16 bit input vector with a 16 bit input
 *	   scale and shift to left with saturation and truncation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   scale		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *	   n			   size of vec1
 *
 *	   shift		   16 bit short signed integer (Shortword) whose value
 *					   falls in the range 0x0000 0000 <= var1 <= 0x0000 1f.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform a multipy of the 16 bit input vector with the 16 bit input
 *	   scale.  The output is a 16 bit vector.
 *
 *	 KEYWORDS: scale
 *
 *************************************************************************/

Shortword *v_scale_shl(Shortword vec1[], Shortword scale, Shortword n,
					   Shortword shift)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = extract_h(L_shl(L_mult(*vec1, scale), shift));
		vec1 ++;
	}
	return(vec1 - n);
}

/***************************************************************************
 *
 *	 FUNCTION NAME: v_sub
 *
 *	 PURPOSE:
 *
 *	   Perform the subtraction of the two 16 bit input vector with
 *	   saturation.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   vec2 		   16 bit short signed integer (Shortword) vector whose
 *					   values falls in the range
 *					   0xffff 8000 <= vec2 <= 0x0000 7fff.
 *
 *	   n			   size of input vectors.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1[] <= 0x0000 7fff.
 *
 *	 IMPLEMENTATION:
 *
 *	   Perform the subtraction of the two 16 bit input vectors with
 *	   saturation.
 *
 *	   vec1 = vec1 - vec2
 *
 *	   vec1[] is set to 0x7fff if the operation results in an
 *	   overflow.  vec1[] is set to 0x8000 if the operation results
 *	   in an underflow.
 *
 *	 KEYWORDS: sub, subtraction
 *
 *************************************************************************/

Shortword *v_sub(Shortword vec1[], Shortword vec2[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = sub(*vec1, *vec2);
		vec1 ++;
		vec2 ++;
	}

	return(vec1 - n);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: v_zap
 *
 *	 PURPOSE:
 *
 *	   Set the elements of a 16 bit input vector to zero.
 *
 *	 INPUTS:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values fall in the range
 *					   0xffff 8000 <= vec1 <= 0x0000 7fff.
 *
 *	   n			   size of vec1.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   vec1 		   16 bit short signed integer (Shortword) vector whose
 *					   values are equal to 0x0000 0000.
 *
 *	 IMPLEMENTATION:
 *
 *	   Set the elements of 16 bit input vector to zero.
 *
 *	   vec1 = 0
 *
 *	 KEYWORDS: zap, clear, reset
 *
 *************************************************************************/
Shortword *v_zap(Shortword vec1[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*vec1 = 0;
		vec1 ++;
	}
	return(vec1 - n);
}


/***************************************************************************
 *
 *	 FUNCTION NAME: L_v_zap
 *
 *	 PURPOSE:
 *
 *	   Set the elements of a 32 bit input vector to zero.
 *
 *	 INPUTS:
 *
 *	   L_vec1		   32 bit long signed integer (Longword) vector whose
 *					   values fall in the range
 *					   0x8000 0000 <= vec1 <= 0x7fff ffff.
 *
 *	   n			   size of L_vec1.
 *
 *	 OUTPUTS:
 *
 *	   none
 *
 *	 RETURN VALUE:
 *
 *	   L_vec1		   32 bit long signed integer (Longword) vector whose
 *					   values are equal to 0x0000 0000.
 *
 *	 IMPLEMENTATION:
 *
 *	   Set the elements of 32 bit input vector to zero.
 *
 *	   L_vec1 = 0
 *
 *	 KEYWORDS: zap, clear, reset
 *
 *************************************************************************/

Longword *L_v_zap(Longword L_vec1[], Shortword n)
{
	register Shortword	i;


	for (i = 0; i < n; i++){
		*L_vec1 = 0;
		L_vec1 ++;
	}

	return(L_vec1 - n);
}


Shortword *v_get(Shortword n)
{
	Shortword	*ptr;
	Longword	size;


	size = sizeof(Shortword) * n;
	ptr = malloc(size);
	assert(ptr != NULL);
	return(ptr);
}


Longword *L_v_get(Shortword n)
{
	Longword	*ptr;
	Longword	size;


	size = sizeof(Longword) * n;
	ptr = malloc(size);
	assert(ptr != NULL);
	return(ptr);
}


void v_free(void *v)
{
	if (v)
		free(v);
}

