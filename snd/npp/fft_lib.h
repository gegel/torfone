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
/* fft_lib.h: FFT function include file */
/* =============================================== */

#ifndef _FFT_LIB_H_
#define _FFT_LIB_H_


/* Radix-2, DIT, 2N-point Real FFT */
void	rfft(Shortword datam1[], Shortword n);
/* Radix-2, DIT, N-point Complex FFT */
Shortword	cfft(Shortword datam1[], Shortword nn);
/* Radix-2, DIT, 256-point Complex FFT */
Shortword	fft_npp(Shortword data[], Shortword dir);

void	fs_init(void);


#endif
