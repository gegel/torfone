/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

/* =============================================== */
/* global.c: global variables for the sc1200 coder */
/* =============================================== */

#include "sc1200.h"


/* ====== Data I/O for high level language implementation ====== */
long	frame_count;
short	rate;

/* ====== Global variables for fixed-point library ====== */
Longword	saturation = 0;
Longword	temp_saturation;

/* ====== General parameters ====== */
struct melp_param	melp_par[NF];                 /* melp analysis parameters */
unsigned char	chbuf[CHSIZE];                     /* channel bit data buffer */
Shortword	frameSize, frameSize12, frameSize24; /* frame size 2.4 = 180 1.2 = 540 */
Shortword	bitNum, bitNum12, bitNum24;                /* number of bits */

/* ====== Quantization ====== */
const Shortword		msvq_bits[MSVQ_STAGES] = {7, 6, 6, 6};
const Shortword		msvq_levels[MSVQ_STAGES] = {128, 64, 64, 64};
struct quant_param	quant_par;

/* ====== Buffers ====== */
Shortword	hpspeech[IN_BEG + BLOCK];       /* input speech buffer dc removed */
Shortword	dcdel[DC_ORD];
Shortword	dcdelin[DC_ORD];
Shortword	dcdelout_hi[DC_ORD];
Shortword	dcdelout_lo[DC_ORD];

/* ====== Classifier ====== */
Shortword	voicedEn, silenceEn;                                       /* Q11 */
Longword	voicedCnt;

/* ====== Fourier Harmonics Weights ====== */
Shortword	w_fs[NUM_HARM];                                            /* Q14 */
Shortword	w_fs_inv[NUM_HARM];
BOOLEAN		w_fs_init = FALSE;

