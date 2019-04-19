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
/* global.h: global variables for the sc1200 coder */
/* =============================================== */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#include "sc1200.h"

/* ====== Data I/O for high level language implementation ====== */
extern long		frame_count;
extern short	rate;

/* ====== Global variables for fixed-point library ====== */
extern Longword		saturation;
extern Longword		temp_saturation;

/* ====== General parameters ====== */
extern struct melp_param	melp_par[];           /* melp analysis parameters */
extern unsigned char	chbuf[];                   /* channel bit data buffer */
extern Shortword	frameSize, frameSize12, frameSize24;
                                                /* frame size 2.4 = 180 1.2 = 540 */
extern Shortword	bitNum, bitNum12, bitNum24;    /* number of bits */

/* ====== Quantization ====== */
extern const Shortword	msvq_bits[];
extern const Shortword	msvq_levels[];
extern struct quant_param	quant_par;

/* ====== Buffers ====== */
extern Shortword	hpspeech[];             /* input speech buffer dc removed */
extern Shortword	dcdel[];
extern Shortword	dcdelin[];
extern Shortword	dcdelout_hi[];
extern Shortword	dcdelout_lo[];

/* ====== Classifier ====== */
extern Shortword	voicedEn, silenceEn;
extern Longword		voicedCnt;

/* ====== Fourier Harmonics Weights ====== */
extern Shortword	w_fs[];
extern Shortword	w_fs_inv[];
extern BOOLEAN		w_fs_init;


// ====== Output bitstream word size ====== */
extern Shortword    chwordsize;
#endif

