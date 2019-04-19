/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

#ifndef _NPP_H_
#define _NPP_H_


/*---------------------------------------------------------------------
 * enfix.h - Speech Enhancement Functions
 *
 * Author: Rainer Martin, AT&T Labs-Research
 *
 * Last Update: $Id: enh_fun.h,v 1.1 1999/02/12 19:07:13 martinr Exp martinr $
 *
 *---------------------------------------------------------------------
 */

#include "sc1200.h"
#include "mathhalf.h"
#include "math_lib.h"
#include "mathdp31.h"


#define SHIFTFRAME 160   //suitable for Europe AMR codec

/********************* SWITCHES FOR COMPILATION ***************************/

/* Note: to reproduce the DAM and DRT evaluation results for versions nsa6 and
		 nsa7 the switches have to be set as follows:
#define USEDOUBLES
#define MINSTAT
#define WRITESHORT
#define NO_APPROX

These settings are also suitable for the latest version (nsa8).
*/

/* define precision: choice of USEDOUBLES or USEFLOATS */
#define USEDOUBLES

/* define the type of noise estimator to be used.
   MINSTAT is the optimal smoothing minimum statistics estimator;
   MALAH   is David Malah's VAD based noise estimation method. */
#define MINSTAT 	  /* choice of MALAH or MINSTAT */

/* define the file format for the enhanced speech: WRITEFLOAT writes the data
   in the Shortword format which might be actually double or float, depending on
   the USEDOUBLES/USEFLOATS switch. WRITESHORT writes 16 Bit short data. */
#define WRITESHORT	 /* choice of WRITESHORT or WRITEFLOAT */

/* select an approximation for the bias computation */
#define NO_APPROX	 /* choice of NO_APPROX and BIAS_APPROX_1 (linear approx.)
						and BIAS_APPROX_2 (quadratic approx.).
						If you try a new frame length and/or new frame shift
						use NO_APPROX and switch later to BIAS_APPROX_2 */


/* ====== Parameters from Noise_Params_Minstat ====== */

#define WTR_FRONT_LEN		32                                          /* Q0 */
#define GM_MIN				3932     /* max value for Min. Gain Modif. Factor */
                                                                 /* 0.12, Q15 */
#define NOISE_B_BY_NSMTH_B	181     /* bias factor for initial noise estimate */
                                                               /* sqrt(2), Q7 */
#define GAMMAX_THR			18102                      /* gamma_max threshold */
                                                     /* 35.35533905932737, Q9 */
#define GAMMAV_THR			23170                       /* gamma_av threshold */
                                                              /* sqrt(2), Q14 */
#define LEN_MINWIN			9                          /* length of subwindow */
#define NUM_MINWIN			8                         /* number of subwindows */
#define ALPHA_N_MAX			30802                  /* maximum of time varying */
                                            /* smoothing parameter, 0.94, Q15 */
#define PDECAY_NUM			-10240                            /* -0.3125, Q15 */
#define MINV				19543                /* parameter used to compute */
                                      /* the minimum bias, 1.19283835953, Q14 */
#define MINV_SUB			13968                     /* 1.7051077144138, Q13 */
#define MINV2				11656                      /* 1.422863351963, Q13 */
#define MINV_SUB2			11909                   /* 2.907392317753447, Q12 */
#define FVAR				16521                      /* 8.066905819615, Q11 */
#define FVAR_SUB			18643                    /* 1.13786305163652, Q14 */

/* ====== Parameters from Enhance_Params ====== */

#define USE_SYN_WINDOW		TRUE                /* switch on synthesis window */

#define ENH_WINLEN			256                               /* frame length */
#define ENH_WIN_SHIFT		SHIFTFRAME    /* frame advance suitable for MELP coder */
#define ENH_OVERLAP_LEN		(ENH_WINLEN - ENH_WIN_SHIFT)
                              /* length of overlapping of adjacent frames, 76 */
#define ENH_VEC_LENF		(ENH_WINLEN/2 + 1)  /* length of spectrum vectors */
#define ENH_NOISE_FRAMES	1          /* number of initial look ahead frames */
#define ENH_NOISE_BIAS		23170              /* noise overestimation factor */
                                                      /* 1.5dB = sqrt(2), Q14 */
#define ENH_INV_NOISE_BIAS	23170                  /* 1.0/ENH_NOISE_BIAS, Q15 */
#define ENH_ALPHA_LT		32023     /* smoothing constant for long term SNR */
                                                             /* 0.977273, Q15 */
#define ENH_BETA_LT			745                              /* 0.022727, Q15 */
#define ENH_QK_MAX			32735    /* Max value for prob. of signal absence */
                                                                /* 0.999, Q15 */
#define ENH_QK_MIN			33       /* Min value for prob. of signal absence */
#define ENH_ALPHAK			30474             /* decision directed ksi weight */
                                                                 /* 0.93, Q15 */
#define ENH_BETAK			18350                                /* 0.07, Q18 */
#define ENH_GAMMAQ_THR		19273                         /* 0.588154886, Q15 */
#define ENH_ALPHAQ			30597        /* Smoothing factor of hard-decision */
                                                    /* qk-value, 0.93375, Q15 */
#define ENH_BETAQ			2171                              /* 0.06625, Q15 */


extern void		npp(Shortword sp_in[], Shortword sp_out[]);
extern void npp_init(void);
void npp_ini(void);

#endif
