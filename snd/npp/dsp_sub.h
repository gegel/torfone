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

/* ======================= */
/* dsp_sub.h: include file */
/* ======================= */

#ifndef _DSP_SUB_H_
#define _DSP_SUB_H_


#include <stdio.h>

void	envelope(Shortword input[], Shortword prev_in, Shortword output[],
				 Shortword npts);

void	fill(Shortword output[], Shortword fillval, Shortword npts);

void	L_fill(Longword output[], Longword fillval, Shortword npts);

void	interp_array(Shortword prev[], Shortword curr[], Shortword out[],
					 Shortword ifact, Shortword size);

Shortword	median3(Shortword input[]);

void	pack_code(Shortword code, unsigned char **ptr_ch_begin,
				  Shortword *ptr_ch_bit, Shortword numbits, Shortword wsize);

Shortword	peakiness(Shortword input[], Shortword npts);

void	quant_u(Shortword *p_data, Shortword *p_index, Shortword qmin,
				Shortword qmax, Shortword nlev, Shortword nlev_q,
				Shortword double_flag, Shortword scale);

void quant_u_dec(Shortword index, Shortword *p_data, Shortword qmin,
				 Shortword qmax, Shortword nlev_q, Shortword scale);

void	rand_num(Shortword output[], Shortword amplitude, Shortword npts);

Shortword rand_minstdgen(void);

Shortword	readbl(Shortword input[], FILE *fp_in, Shortword size);

BOOLEAN	unpack_code(unsigned char **ptr_ch_begin, Shortword *ptr_ch_bit,
					Shortword *code, Shortword numbits, Shortword wsize,
					UShortword erase_mask);

void	window_M(Shortword input[], const Shortword win_coeff[],
			   Shortword output[], Shortword npts);

void	window_Q(Shortword input[], Shortword win_coeff[], Shortword output[],
				 Shortword npts, Shortword Qin);

void	writebl(Shortword output[], FILE *fp_out, Shortword size);

void	polflt(Shortword input[], Shortword coeff[], Shortword output[],
			   Shortword order, Shortword npts);

void	zerflt(Shortword input[], const Shortword coeff[], Shortword output[],
			   Shortword order, Shortword npts);

void	zerflt_Q(Shortword input[], const Shortword coeff[],
				 Shortword output[], Shortword order, Shortword npts,
				 Shortword Q_coeff);

void	iir_2nd_d(Shortword input[], const Shortword den[],
				  const Shortword num[], Shortword output[], Shortword delin[],
				  Shortword delout_hi[], Shortword delout_lo[],
				  Shortword npts);

void	iir_2nd_s(Shortword input[], const Shortword den[],
				  const Shortword num[], Shortword output[],
				  Shortword delin[], Shortword delout[], Shortword npts);

Shortword	interp_scalar(Shortword prev, Shortword curr, Shortword ifact);


#endif

