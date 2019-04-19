/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

/*---------------------------------------------------------------------
 * enh_fun.c - Speech Enhancement Functions
 *
 * Author: Rainer Martin, AT&T Labs-Research
 *
 * Last Update: $Id: enh_fun.c,v 1.2 1999/02/19 17:55:12 martinr Exp martinr $
 *
 *---------------------------------------------------------------------
 */

#include "npp.h"
#include "fs_lib.h"
#include "mat_lib.h"
#include "constant.h"
#include "dsp_sub.h"
#include "fft_lib.h"
#include "global.h"


#define X003_Q15		983                               /* 0.03 * (1 << 15) */
#define X005_Q15		1638                              /* 0.05 * (1 << 15) */
#define X006_Q15		1966                              /* 0.06 * (1 << 15) */
#define X018_Q15		5898                              /* 0.18 * (1 << 15) */
#define X065_Q15		21299                             /* 0.65 * (1 << 15) */
#define X132_Q11		2703                              /* 1.32 * (1 << 11) */
#define X15_Q13			12288                              /* 1.5 * (1 << 13) */
#define X22_Q11			4506                               /* 2.2 * (1 << 11) */
#define X44_Q11			9011                               /* 4.4 * (1 << 11) */
#define X88_Q11			18022                              /* 8.8 * (1 << 11) */

const static Shortword	sqrt_tukey_256_180[ENH_WINLEN] = {             /* Q15 */
	  677,	1354,  2030,  2705,  3380,  4053,  4724,  5393,
	 6060,  6724,  7385,  8044,  8698,  9349,  9996, 10639,
	11277, 11911, 12539, 13162, 13780, 14391, 14996, 15595,
	16188, 16773, 17351, 17922, 18485, 19040, 19587, 20126,
	20656, 21177, 21690, 22193, 22686, 23170, 23644, 24108,
	24561, 25004, 25437, 25858, 26268, 26668, 27056, 27432,
	27796, 28149, 28490, 28818, 29134, 29438, 29729, 30008,
	30273, 30526, 30766, 30992, 31205, 31405, 31592, 31765,
	31924, 32070, 32202, 32321, 32425, 32516, 32593, 32656,
	32705, 32740, 32761, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767,
	32767, 32767, 32767, 32767, 32761, 32740, 32705, 32656,
	32593, 32516, 32425, 32321, 32202, 32070, 31924, 31765,
	31592, 31405, 31205, 30992, 30766, 30526, 30273, 30008,
	29729, 29438, 29134, 28818, 28490, 28149, 27796, 27432,
	27056, 26668, 26268, 25858, 25437, 25004, 24561, 24108,
	23644, 23170, 22686, 22193, 21690, 21177, 20656, 20126,
	19587, 19040, 18485, 17922, 17351, 16773, 16188, 15595,
	14996, 14391, 13780, 13162, 12539, 11911, 11277, 10639,
	 9996,	9349,  8698,  8044,  7385,  6724,  6060,  5393,
	 4724,  4053,  3380,  2705,  2030,  1354,   677,     0
};

/* ====== Entities from Enhanced_Data ====== */
static Shortword	enh_i = 0;                               /* formerly D->I */
static Shortword	lambdaD[ENH_VEC_LENF];             /* overestimated noise */
                                               /* psd(noise_bias * noisespect)*/
static Shortword	lambdaD_shift[ENH_VEC_LENF];
static Shortword	SN_LT;                                   /* long term SNR */
static Shortword	SN_LT_shift;
static Shortword	n_pwr;
static Shortword	n_pwr_shift;
static Shortword    YY[ENH_VEC_LENF];  /* signal periodogram of current frame */
static Shortword	YY_shift[ENH_VEC_LENF];
static Shortword	sm_shift[ENH_VEC_LENF];
static Shortword	noise_shift[ENH_VEC_LENF];
static Shortword	noise2_shift[ENH_VEC_LENF];
static Shortword	av_shift[ENH_VEC_LENF];
static Shortword	av2_shift[ENH_VEC_LENF];
static Shortword	act_min[ENH_VEC_LENF];  /* current minimum of long window */
static Shortword	act_min_shift[ENH_VEC_LENF];
static Shortword	act_min_sub[ENH_VEC_LENF];
                                             /* current minimum of sub-window */
static Shortword	act_min_sub_shift[ENH_VEC_LENF];
static Shortword	vk[ENH_VEC_LENF];
static Shortword	vk_shift[ENH_VEC_LENF];
static Shortword	ksi[ENH_VEC_LENF];                        /* a priori SNR */
static Shortword	ksi_shift[ENH_VEC_LENF];

static Shortword	var_rel[ENH_VEC_LENF];
                                       /* est. relative var. of smoothedspect */
static Shortword	var_rel_av;     /* average relative var. of smoothedspect */

/* only for minimum statistics */

static Shortword	smoothedspect[ENH_VEC_LENF];  /* smoothed signal spectrum */
static Shortword    var_sp_av[ENH_VEC_LENF];
                                           /* estimated mean of smoothedspect */
static Shortword    var_sp_2[ENH_VEC_LENF];
                                     /* esitmated 2nd moment of smoothedspect */
static Shortword	noisespect[ENH_VEC_LENF];          /* estimated noise psd */
static Shortword	noisespect2[ENH_VEC_LENF];
static Shortword	alphacorr;        /* correction factor for alpha_var, Q15 */
static Shortword	alpha_var[ENH_VEC_LENF];            /* variable smoothing */
                                                         /* parameter for psd */
static Shortword	circb[NUM_MINWIN][ENH_VEC_LENF];           /* ring buffer */
static Shortword	circb_shift[NUM_MINWIN][ENH_VEC_LENF];

static Shortword	initial_noise[ENH_WINLEN];            /* look ahead noise */
                                                              /* samples (Q0) */
static Shortword	speech_in_npp[ENH_WINLEN];          /* input of one frame */
static Shortword	ybuf[2*ENH_WINLEN + 2];
                                 /* buffer for FFT, this can be eliminated if */
						    /* we can write a better real-FFT program for DSP */
static Longword		temp_yy[ENH_WINLEN + 2];

/* ====== Prototypes ====== */

static void		smoothing_win(Shortword initial_noise[]);
static void		compute_qk(Shortword qk[], Shortword gamaK[],
						   Shortword gamaK_shift[], Shortword GammaQ_TH);
static void		gain_log_mmse(Shortword qk[], Shortword Gain[],
							  Shortword gamaK[], Shortword gamaK_shift[],
							  Shortword m);
static Shortword	ksi_min_adapt(BOOLEAN n_flag, Shortword ksi_min,
                                  Shortword sn_lt, Shortword sn_lt_shift);
static void		smoothed_periodogram(Shortword YY_av, Shortword yy_shift);
static void		bias_compensation(Shortword biased_spect[],
								  Shortword bias_shift[],
								  Shortword biased_spect_sub[],
								  Shortword bias_sub_shift[]);
static Shortword	noise_slope(void);
static Shortword	comp_data_shift(Shortword num1, Shortword shift1,
									Shortword num2, Shortword shift2);
static void		min_search(Shortword biased_spect[], Shortword bias_shift[],
						   Shortword biased_spect_sub[],
						   Shortword bias_sub_shift[]);
void			enh_init(void);
static void		minstat_init(void);
static void		process_frame(Shortword inspeech[], Shortword outspeech[]);
static void		gain_mod(Shortword qk[], Shortword GainD[], Shortword m);


/****************************************************************************
**
** Function:		npp
**
** Description: 	The noise pre-processor
**
** Arguments:
**
**	Shortword	sp_in[]  ---- input speech data buffer (Q0)
**	Shortword	sp_out[] ---- output speech data buffer (Q0)
**
** Return value:	None
**
*****************************************************************************/
void	npp(Shortword sp_in[], Shortword sp_out[])
{
	static Shortword	speech_overlap[ENH_OVERLAP_LEN];
	static BOOLEAN	first_time = TRUE;
	Shortword   speech_out_npp[ENH_WINLEN];            /* output of one frame */


	if (first_time){
	
		if (rate == RATE1200)
			v_equ(initial_noise, sp_in, ENH_WINLEN);
		else {
			v_zap(initial_noise, ENH_OVERLAP_LEN);
			v_equ(&initial_noise[ENH_OVERLAP_LEN], sp_in, ENH_WIN_SHIFT);
		}
		enh_init();                         /* Initialize enhancement routine */
		v_zap(speech_in_npp, ENH_WINLEN);

		first_time = FALSE;
	}

	/* Shift input buffer from the previous frame */
	v_equ(speech_in_npp, &(speech_in_npp[ENH_WIN_SHIFT]), ENH_OVERLAP_LEN);
	v_equ(&(speech_in_npp[ENH_OVERLAP_LEN]), sp_in, ENH_WIN_SHIFT); 

	/* ======== Process one frame ======== */
	process_frame(speech_in_npp, speech_out_npp);

	/* Overlap-add the output buffer. */
	v_add(speech_out_npp, speech_overlap, ENH_OVERLAP_LEN);
	v_equ(speech_overlap, &(speech_out_npp[ENH_WIN_SHIFT]), ENH_OVERLAP_LEN);

	/* ======== Output enhanced speech ======== */
	v_equ(sp_out, speech_out_npp, ENH_WIN_SHIFT);
}


/*****************************************************************************/
/*	  Subroutine gain_mod: compute gain modification factor based on		 */
/*		 signal absence probabilities qk									 */
/*****************************************************************************/
static void		gain_mod(Shortword qk[], Shortword GainD[], Shortword m)
{
	register Shortword	i;
	Shortword	temp, temp1, temp2, temp3, temp4, shift;
	Shortword	temp_shift, temp2_shift;
	Longword	L_sum, L_tmp;


	for (i = 0; i < m; i++){
		/*	temp = 1.0 - qk[i]; */
		temp = sub(ONE_Q15, qk[i]);                                    /* Q15 */
		if (temp == 0)
			temp = 1;
		shift = norm_s(temp);
		temp = shl(temp, shift);
		temp_shift = negate(shift);

		/*	temp2 = temp*temp; */
		L_sum = L_mult(temp, temp);
		shift = norm_l(L_sum);
		temp2 = extract_h(L_shl(L_sum, shift));
		temp2_shift = sub(shl(temp_shift, 1), shift);

		/*	GM[i] = temp2 / (temp2 + (temp + ksi[i])*qk[i]*exp(-vk[i])) */

		/*	exp(-vk) = 2^(2*vk*(-0.5*log2(e))), and -23637 is -0.5*log2(e).   */
		L_sum = L_mult(vk[i], -23637);              /* vk*(-0.5*log2(e)), Q15 */
		shift = add(vk_shift[i], 1);
		L_sum = L_shr(L_sum, sub(15, shift));                    /* 2^x x Q16 */

		/*	temp + ksi[i] */
		shift = sub(ksi_shift[i], temp_shift);
		if (shift > 0){
			temp4 = add(ksi_shift[i], 1);
			temp3 = add(shr(temp, add(shift, 1)), shr(ksi[i], 1));
		} else {
			temp4 = add(temp_shift, 1);
			temp3 = add(shr(temp, 1), shl(ksi[i], sub(shift, 1)));
		}
		/*	(temp + ksi[i])*qk[i] */
		L_tmp = L_mult(temp3, qk[i]);

		/*	temp + ksi[i])*qk[i]*exp(-vk[i] */
		L_sum = L_add(L_sum, L_deposit_h(temp4));                  /* add exp */
		shift = extract_h(L_sum);                         /* this is exp part */

		temp4 = (Shortword) (extract_l(L_shr(L_sum, 1)) & 0x7fff);
		temp1 = shr(mult(temp4, 9864), 3);           /* change to base 10 Q12 */
		temp1 = pow10_fxp(temp1, 14);                              /* out Q14 */
		L_tmp = L_mpy_ls(L_tmp, temp1);                                /* Q30 */
		temp3 = norm_l(L_tmp);
		temp1 = extract_h(L_shl(L_tmp, temp3));
		shift = add(shift, sub(1, temp3));                  /* make L_tmp Q31 */

		/*	temp2 + (temp + ksi[i])*qk[i]*exp(-vk[i]) */
		temp1 = shr(temp1, 1);
		temp2 = shr(temp2, 1);
		temp = sub(shift, temp2_shift);
		if (temp > 0){
			temp3 = add(temp1, shr(temp2, temp));
			temp4 = shr(temp2, temp);
		} else {
			temp3 = add(shl(temp1, temp), temp2);
			temp4 = temp2;
		}

		temp = divide_s(temp4, temp3);   /* temp is previously known as GM[]. */

		/* limit lowest GM value */
		if (temp < GM_MIN)
			temp = GM_MIN;                                             /* Q15 */
		GainD[i] = mult(GainD[i], temp);                     /* modified gain */
	}
}


/*****************************************************************************/
/*	  Subroutine compute_qk: compute the probability of speech absence       */
/*		This uses an harddecision approach due to Malah (ICASSP 1999).		 */
/*****************************************************************************/
static void		compute_qk(Shortword qk[], Shortword gamaK[],
						   Shortword gamaK_shift[], Shortword GammaQ_TH)
{
	register Shortword	i;
	static BOOLEAN		first_time = TRUE;
	static Shortword	qla[ENH_VEC_LENF];


	/* Initializing qla[] */
	if (first_time){
		fill(qla, X05_Q15, ENH_VEC_LENF);
		first_time = FALSE;
	}

	/*	qla[] = alphaq * qla[]; */
	v_scale(qla, ENH_ALPHAQ, ENH_VEC_LENF);
	for (i = 0; i < ENH_VEC_LENF; i++){

		/*	if (gamaK[i] < GammaQ_TH) */
		if (comp_data_shift(gamaK[i], gamaK_shift[i], GammaQ_TH, 0) < 0)
			/*	qla[] += betaq; */
			qla[i] = add(qla[i], ENH_BETAQ);
	}
	v_equ(qk, qla, ENH_VEC_LENF);
}


/*****************************************************************************/
/*	  Subroutine gain_log_mmse: compute the gain factor and the auxiliary	 */
/*		variable vk for the Ephraim&Malah 1985 log spectral estimator.		 */
/*	  Approximation of the exponential integral due to Malah, 1996			 */
/*****************************************************************************/
static void		gain_log_mmse(Shortword qk[], Shortword Gain[],
							  Shortword gamaK[], Shortword gamaK_shift[],
							  Shortword m)
{
	register Shortword	i;
	Shortword	ksi_vq, temp1, temp2, shift;
	Longword	L_sum;


	for (i = 0; i < m; i++){

		/*	1.0 - qk[] */
		temp1 = sub(ONE_Q15, qk[i]);
		shift = norm_s(temp1);
		temp1 = shl(temp1, shift);
		temp2 = sub(ksi_shift[i], (Shortword) (-shift));
		/*	ksi[] + 1.0 - qk[] */
		if (temp2 > 0){
			temp1 = shr(temp1, add(temp2, 1));
			temp1 = add(temp1, shr(ksi[i], 1));
			temp2 = shr(ksi[i], 1);
		} else {
			temp1 = add(shr(temp1,1), shl(ksi[i], sub(temp2, 1)));
			temp2 = shl(ksi[i], sub(temp2, 1));
		}

		/*	ksi_vq = ksi[i] / (ksi[i] + 1.0 - qk[i]); */
		ksi_vq = divide_s(temp2, temp1);                               /* Q15 */

		/*	vk[i] = ksi_vq * gamaK[i]; */
		L_sum = L_mult(ksi_vq, gamaK[i]);
		shift = norm_l(L_sum);
		vk[i] = extract_h(L_shl(L_sum, shift));
		vk_shift[i] = sub(gamaK_shift[i], shift);

		/* The original floating version compares vk[] against 2^{-52} ==     */
		/* 2.220446049250313e-16.  Tian uses 32767*2^{-52} instead.           */
		if (comp_data_shift(vk[i], vk_shift[i], 32767, -52) < 0){
			vk[i] = 32767;                                      /* MATLAB eps */
			vk_shift[i] = -52;
		}

		if (comp_data_shift(vk[i], vk_shift[i], 26214, -3) < 0){
			/* eiv = -2.31 * log10(vk[i]) - 0.6; */
			temp1 = log10_fxp(vk[i], 15);                              /* Q12 */
			L_sum = L_shl(L_deposit_l(temp1), 14);                     /* Q26 */
			L_sum = L_add(L_sum, L_shl(L_mult(vk_shift[i], 9864), 10));
			L_sum = L_mpy_ls(L_sum, -18923);
                                             /* -18923 = -2.31 (Q13). out Q24 */
			L_sum = L_sub(L_sum, 10066330L);    /* 10066330L = 0.6 (Q24), Q24 */
		} else if (comp_data_shift(vk[i], vk_shift[i], 25600, 8) > 0){
			/*	eiv = pow(10, -0.52 * 200 - 0.26); */
			L_sum = 1;                                                 /* Q24 */

			/*	vk[] = 200; */
			vk[i] = 25600;
			vk_shift[i] = 8;
		} else if (comp_data_shift(vk[i], vk_shift[i], 32767, 0) > 0){
			/*	eiv = pow(10, -0.52 * vk[i] - 0.26); */
			L_sum = L_mult(vk[i], -17039);             /* -17039 == -0.52 Q15 */
			L_sum = L_sub(L_sum, L_shr(L_deposit_h(8520), vk_shift[i]));
                                                          /* 8520 == 0.26 Q15 */
			L_sum = L_shr(L_sum, sub(14, vk_shift[i]));
			L_sum = L_mpy_ls(L_sum, 27213);                  /* Q15 to base 2 */
			shift = extract_h(L_shl(L_sum, 1));               /* integer part */
			temp1 = (Shortword) (extract_l(L_sum) & 0x7fff);
			temp1 = shr(mult(temp1, 9864), 3);              /* Q12 to base 10 */
			temp1 = pow10_fxp(temp1, 14);                       /* output Q14 */
			L_sum = L_shl(L_deposit_l(temp1), 10);                 /* Q24 now */
			L_sum = L_shl(L_sum, shift);                    /* shift must < 0 */
		} else {
			/* eiv = -1.544 * log10(vk[i]) + 0.166; */
			temp1 = vk[i];
			if (vk_shift[i] != 0)
				temp1 = shl(temp1, vk_shift[i]);
			temp1 = log10_fxp(temp1, 15);                              /* Q12 */
			L_sum = L_shl(L_deposit_l(temp1), 13);                     /* Q25 */
			L_sum = L_mpy_ls(L_sum, -25297);     /* -25297 = -1.544, Q14. Q24 */
			L_sum = L_add(L_sum, 2785018L);         /* 2785018L == 0.166, Q24 */
		}

		/* Now "eiv" is kept in L_sum. */
		/*	Gain[i] = ksi_vq * exp (0.5 * eiv); */

		L_sum = L_mpy_ls(L_sum, 23637);                  /* 0.72135 to base 2 */
		shift = shr(extract_h(L_sum), 8);                        /* get shift */
		if (comp_data_shift(ksi_vq, shift, 32767, 0) > 0){
			Gain[i] = 32767;
			continue;
		} else {
			temp1 = extract_l(L_shr(L_sum, 9));
			temp1 = (Shortword) (temp1 & 0x7fff);
			temp1 = shr(mult(temp1, 9864), 3);       /* change to base 10 Q12 */
			temp1 = pow10_fxp(temp1, 14);

			L_sum = L_shl(L_deposit_h(ksi_vq), shift);
			L_sum = L_mpy_ls(L_sum, temp1);
			if (L_sub(L_sum, 1073676288L) > 0)
				Gain[i] = 32767;
			else
				Gain[i] = extract_h(L_shl(L_sum, 1));
		}
	}
}


/*****************************************************************************/
/*	   Subroutine ksi_min_adapt: computes the adaptive ksi_min				 */
/*****************************************************************************/
static Shortword	ksi_min_adapt(BOOLEAN n_flag, Shortword ksi_min,
								  Shortword sn_lt, Shortword sn_lt_shift)
{
	Shortword	ksi_min_new, temp, shift;
	Longword	L_sum;


	if (n_flag)             /* RM: adaptive modification of ksi limit (10/98) */
		ksi_min_new = ksi_min;
	else {
		if (sn_lt_shift > 0){
			L_sum = L_add(L_deposit_l(sn_lt), L_shr(X05_Q15, sn_lt_shift));
			shift = sn_lt_shift;
		} else {
			L_sum = L_add(L_shl(L_deposit_l(sn_lt), sn_lt_shift), X05_Q15);
			shift = 0;
		}

		/* L_sum is (sn_lt * 2^sn_lt_shift + 0.5) */
		if (L_sum > ONE_Q15){
			L_sum = L_shr(L_sum, 1);
			shift = add(shift, 1);
		}
		temp = extract_l(L_sum);

		/* We want to compute 2^{0.65*[log2(0.5+sn_lt)]-7.2134752} */
		/* equiv. 2^{0.65*log10(temp)/log10(2) + 0.65*shift - 7.2134752} */
		temp = log10_fxp(temp, 15);                 /* log(0.5 + sn_lt), Q12 */
		L_sum = L_shr(L_mult(temp, 8844), 9);       /* Q12*Q12->Q25 --> Q16 */

		L_sum = L_add(L_sum, L_mult(shift, 21299)); /* 21299 = 0.65 Q15 */
		L_sum = L_sub(L_sum, 472742L);              /* 472742 = 7.2134752 Q16 */
		shift = extract_h(L_sum);                   /* the pure shift */
		temp = (Shortword) (extract_l(L_shr(L_sum, 1)) & 0x7fff);
		temp = mult(temp, 9864);                    /* change to base 10 */
		temp = shr(temp, 3);                        /* change to Q12 */
		temp = pow10_fxp(temp, 14);
		L_sum = L_shl(L_mult(ksi_min, temp), 1);    /* Now Q31 */
		temp = extract_h(L_sum);

		/*	if (ksi_min_new > 0.25) */
		if (comp_data_shift(temp, shift, 8192, 0) > 0)
			ksi_min_new = 8192;
		else
			ksi_min_new = shl(temp, shift);
	}

	return(ksi_min_new);
}


/*****************************************************************************/
/* Subroutine smoothing_win: applies the Parzen window.  The window applies  */
/* an inverse trapezoid window and wtr_front[] supplies the coefficients for */
/* the two edges.                                                            */
/*****************************************************************************/
static void		smoothing_win(Shortword initial_noise[])
{
	register Shortword	i;
	const static Shortword	wtr_front[WTR_FRONT_LEN] = {               /* Q15 */
		32767, 32582, 32048, 31202, 30080, 28718, 27152, 25418,
		23552, 21590, 19568, 17522, 15488, 13502, 11600,  9818,
		 8192,  6750,  5488,  4394,  3456,  2662,  2000,  1458,
		 1024,   686,   432,   250,   128,    54,    16,     2
	};


	for (i = 1; i < WTR_FRONT_LEN; i++)
		initial_noise[i] = mult(initial_noise[i], wtr_front[i]);
	for (i = ENH_WINLEN - WTR_FRONT_LEN + 1; i < ENH_WINLEN; i++)
		initial_noise[i] = mult(initial_noise[i], wtr_front[ENH_WINLEN - i]);

	/* Clearing the central part of initial_noise[]. */
	v_zap(&(initial_noise[WTR_FRONT_LEN]), ENH_WINLEN - 2*WTR_FRONT_LEN + 1);
}


/***************************************************************************/
/*	Subroutine smoothed_periodogram: compute short time psd with optimal   */
/*						 smoothing										   */
/***************************************************************************/
static void		smoothed_periodogram(Shortword YY_av, Shortword yy_shift)
{
	register Shortword	i;
	Shortword	smoothed_av, alphacorr_new, alpha_N_min_1, alpha_num;
	Shortword	smav_shift, shift, temp, temp1, tmpns, tmpalpha;
	Shortword	noise__shift, temp_shift, tmpns_shift, max_shift;
	Longword	L_sum, L_tmp; //,L_max, ;


	/* ---- compute smoothed_av ---- */
	max_shift = SW_MIN;
	for (i = 0; i < ENH_VEC_LENF; i++){
		if (sm_shift[i] > max_shift)
			max_shift = sm_shift[i];
	}

	L_sum = L_shl(L_deposit_l(smoothedspect[0]),
				  sub(7, sub(max_shift, sm_shift[0])));
	L_sum = L_add(L_sum, L_shl(L_deposit_l(smoothedspect[ENH_VEC_LENF - 1]),
				  sub(7, sub(max_shift, sm_shift[ENH_VEC_LENF - 1]))));
	for (i = 1; i < ENH_VEC_LENF - 1; i++)
		L_sum = L_add(L_sum, L_shl(L_deposit_l(smoothedspect[i]),
					  sub(8, sub(max_shift, sm_shift[i]))));

	/* Now L_sum contains smoothed_av.  Here we do not multiply L_sum with    */
	/* win_len_inv because we do not do this on YY_av either.                 */

	if (L_sum == 0)
		L_sum = 1;
	temp = norm_l(L_sum);
	temp = sub(temp, 1);                     /* make sure smoothed_av < YY_av */
	smoothed_av = extract_h(L_shl(L_sum, temp));
	smav_shift = sub(add(max_shift, 1), temp);

	/* ---- alphacorr_new = smoothed_av / YY_av - 1.0 ---- */
	alphacorr_new = divide_s(smoothed_av, YY_av);
	shift = sub(smav_shift, yy_shift);
	if (shift <= 0){
		if (shift > -15)
			alphacorr_new = sub(shl(alphacorr_new, shift), ONE_Q15);
		else
			alphacorr_new = negate(ONE_Q15);
		shift = 0;
	} else {
		if (shift < 15)
			alphacorr_new = sub(alphacorr_new, shr(ONE_Q15, shift));
	}

	/* -- alphacorr_new = 1.0 / (1.0 + alphacorr_new * alphacorr_new) -- */
	alphacorr_new = mult(alphacorr_new, alphacorr_new);
	alphacorr_new = shr(alphacorr_new, 1);          /* avoid overflow when +1 */
	shift = shl(shift, 1);
	if (shift < 15)
		alphacorr_new = add(alphacorr_new, shr(ONE_Q14, shift));
	if (alphacorr_new == 0)
		alphacorr_new = ONE_Q15;
	else {
		if (shift < 15){
			temp = shr(ONE_Q14, shift);
			alphacorr_new = divide_s(temp, alphacorr_new);
		} else                                                   /* too small */
			alphacorr_new = 0;
	}

	/* -- alphacorr_new > 0.7 ? alphacorr_new : 0.7 -- */
	if (alphacorr_new < X07_Q15)
		alphacorr_new = X07_Q15;

	/* -- alphacorr = 0.7*alphacorr + 0.3*alphacorr_new -- */
	alphacorr = extract_h(L_add(L_mult(X07_Q15, alphacorr),
								L_mult(X03_Q15, alphacorr_new)));

	/* -- compute alpha_N_min_1 -- */
	/* -- alpha_N_min_1 = pow(SN_LT, PDECAY_NUM) */
	if (comp_data_shift(SN_LT, SN_LT_shift, 16384, 15) > 0)
		L_sum = 536870912L;                                            /* Q15 */
	else if (comp_data_shift(SN_LT, SN_LT_shift, 32, 15) < 0)
		L_sum = 1048576L;
	else
		L_sum = L_shl(L_deposit_l(SN_LT), SN_LT_shift);
	temp = L_log10_fxp(L_sum, 15);                              /* output Q11 */
	temp = shl(temp, 1);                                    /* convert to Q12 */
	temp = mult(temp, (Shortword) PDECAY_NUM);
	alpha_N_min_1 = pow10_fxp(temp, 15);

	/* -- alpha_N_min_1 = (0.3 < alpha_N_min_1 ? 0.3 : alpha_N_min_1) -- */
	/* -- alpha_N_min_1 = (alpha_N_min_1 < 0.05 ? 0.05 : alpha_N_min_1) -- */
	if (alpha_N_min_1 > X03_Q15)
		alpha_N_min_1 = X03_Q15;
	else if (alpha_N_min_1 < X005_Q15)
		alpha_N_min_1 = X005_Q15;

	/* -- alpha_num = ALPHA_N_MAX * alphacorr -- */
	alpha_num = mult(ALPHA_N_MAX, alphacorr);

	/* -- compute smoothed spectrum -- */
	//L_max = 0;
	for (i = 0; i < ENH_VEC_LENF; i++){
		tmpns = noisespect[i];

		/*	temp = smoothedspect[i] - tmpns; */
		shift = sub(sm_shift[i], noise_shift[i]);
		if (shift > 0){
			L_tmp = L_sub(L_deposit_h(smoothedspect[i]),
						  L_shr(L_deposit_h(noisespect[i]), shift));
			shift = sm_shift[i];
		} else {
			L_tmp = L_sub(L_shr(L_deposit_h(smoothedspect[i]), abs_s(shift)),
						  L_deposit_h(tmpns));
			shift = noise_shift[i];
		}

		temp1 = norm_l(L_tmp);
		temp = extract_h(L_shl(L_tmp, temp1));
		shift = sub(shift, temp1);

		/*	tmpns = tmpns * tmpns; */
		L_sum = L_mult(tmpns, tmpns);                /* noise square, shift*2 */
		noise__shift = norm_l(L_sum);
		tmpns = extract_h(L_shl(L_sum, noise__shift));
		tmpns_shift = sub(shl(noise_shift[i], 1), noise__shift);
		noisespect2[i] = tmpns;
		noise2_shift[i] = tmpns_shift;

		if (temp == SW_MIN)
			L_sum = 0x7fffffff;
		else
			L_sum = L_mult(temp, temp);
		noise__shift = norm_l(L_sum);
		temp = extract_h(L_shl(L_sum, noise__shift));
		if (temp == 0)
			temp_shift = -20;
		else
			temp_shift = sub(shl(shift, 1), noise__shift);

		/* -- tmpalpha = alpha_num * tmpns / (tmpns + temp * temp) -- */
		temp1 = sub(temp_shift, tmpns_shift);
		if (temp1 > 0){
			temp = shr(temp, 1);
			tmpns = shr(tmpns, add(temp1, 1));
		} else {
			tmpns = shr(tmpns, 1);
			temp = shl(temp, sub(temp1, 1));
		}
		tmpalpha = divide_s(tmpns, add(temp, tmpns));
		tmpalpha = mult(tmpalpha, alpha_num);

		/* tmpalpha = (tmpalpha < alpha_N_min_1 ? alpha_N_min_1 : tmpalpha) */
		if (tmpalpha < alpha_N_min_1)
			tmpalpha = alpha_N_min_1;

		temp = sub(ONE_Q15, tmpalpha);                        /* 1 - tmpalpha */
		alpha_var[i] = tmpalpha;                                /* save alpha */

		/*	smoothedspect[i] = tmpalpha * smoothedspect[i] + */
		/*                     (1 - tmpalpha) * YY[i]; */
		smav_shift = sub(YY_shift[i], sm_shift[i]);
		if (smav_shift > 0){
			L_sum = L_shr(L_mult(tmpalpha, smoothedspect[i]), smav_shift);
			L_sum = L_add(L_sum, L_mult(temp, YY[i]));
			sm_shift[i] = YY_shift[i];
		} else {
			L_sum = L_mult(tmpalpha, smoothedspect[i]);
			L_sum = L_add(L_sum, L_shl(L_mult(temp, YY[i]), smav_shift));
		}
		if (L_sum < 1)
			L_sum = 1;
		shift = norm_l(L_sum);
		smoothedspect[i] = extract_h(L_shl(L_sum, shift));
		sm_shift[i] = sub(sm_shift[i], shift);
	}
}


/*****************************************************************************/
/* Subroutine normalized_variance: compute variance of smoothed periodogram, */
/* normalize it, and use it to compute a biased smoothed periodogram		 */
/*****************************************************************************/
static void		bias_compensation(Shortword biased_spect[],
								  Shortword bias_shift[],
								  Shortword biased_spect_sub[],
								  Shortword bias_sub_shift[])
{
	register Shortword	i;
	Shortword	tmp, tmp1, tmp2, tmp5;
	Shortword	beta_var, var_rel_av_sqrt;
	Shortword	av__shift, av2__shift, shift1, shift3, shift4;
	Longword	L_max1, L_max2, L_sum, L_var_sum, L_tmp3, L_tmp4;


	/* ---- compute variance of smoothed psd ---- */
	L_var_sum = 0;
	for (i = 0; i < ENH_VEC_LENF; i++){

		/*	tmp1 = alpha_var[i]*alpha_var[i]; */
		/*	beta_var = (tmp1 > 0.8 ? 0.8 : tmp1); */
		beta_var = mult(alpha_var[i], alpha_var[i]);
		if (beta_var > X08_Q15)
			beta_var = X08_Q15;

		/*	tmp2 = (1 - beta_var) * smoothedspect[i]; */
		L_sum = L_mult(sub(ONE_Q15, beta_var), smoothedspect[i]);

		/*	var_sp_av[i] = beta_var * var_sp_av[i] + tmp2; */
		av__shift = sub(sm_shift[i], av_shift[i]);
		if (av__shift > 0){
			L_max1 = L_add(L_shr(L_mult(beta_var, var_sp_av[i]), av__shift),
						   L_sum);
			av_shift[i] = sm_shift[i];
		} else
			L_max1 = L_add(L_mult(beta_var, var_sp_av[i]),
						   L_shl(L_sum, av__shift));
		if (L_max1 < 1)
			L_max1 = 1;
		shift1 = norm_l(L_max1);
		var_sp_av[i] = extract_h(L_shl(L_max1, shift1));
		av_shift[i] = sub(av_shift[i], shift1);

		/*	tmp4 = tmp2 * smoothedspect[i]; */
		/*	var_sp_2[i] = beta_var * var_sp_2[i] + tmp4; */
		av2__shift = sub(shl(sm_shift[i], 1), av2_shift[i]);
		if (av2__shift > 0){
			L_max2 = L_add(L_shr(L_mult(beta_var, var_sp_2[i]), av2__shift),
						   L_mpy_ls(L_sum, smoothedspect[i]));
			av2_shift[i] = shl(sm_shift[i], 1);
		} else
			L_max2 = L_add(L_mult(beta_var, var_sp_2[i]),
						   L_shl(L_mpy_ls(L_sum, smoothedspect[i]),
								 av2__shift));

		if (L_max2 < 1)
			L_max2 = 1;
		shift1 = norm_l(L_max2);
		var_sp_2[i] = extract_h(L_shl(L_max2, shift1));
		av2_shift[i] = sub(av2_shift[i], shift1);

		/*	tmp3 = var_sp_av[i] * var_sp_av[i]; */
		L_sum = L_mult(var_sp_av[i], var_sp_av[i]);

		/*	var_sp = var_sp_2[i] - tmp3; */
		shift3 = sub(av2_shift[i], shl(av_shift[i], 1));
		if (shift3 > 0){
			L_sum = L_sub(L_deposit_h(var_sp_2[i]), L_shr(L_sum, shift3));
			shift4 = av2_shift[i];
		} else {
			L_sum = L_sub(L_shl(L_deposit_h(var_sp_2[i]), shift3), L_sum);
			shift4 = shl(av_shift[i], 1);
		}
		shift1 = sub(norm_l(L_sum), 1);
		tmp1 = extract_h(L_shl(L_sum, shift1));
		tmp = sub(sub(shift4, shift1), noise2_shift[i]);

		/*	tmp1 = var_sp / noisespect2[i]; */
		var_rel[i] = divide_s(tmp1, noisespect2[i]);

		/*	var_rel[i] = (tmp1 > 0.5 ? 0.5 : tmp1); */
		if (comp_data_shift(var_rel[i], tmp, X05_Q15, 0) > 0)
			var_rel[i] = X05_Q15;
		else
			var_rel[i] = shl(var_rel[i], tmp);
		if (var_rel[i] < 0)
			var_rel[i] = 0;

		/*	var_sum += var_rel[i]; */
		L_var_sum = L_add(L_var_sum, L_deposit_l(var_rel[i]));
	}

	/*	var_rel_av = (2 * var_sum - var_rel[0] - var_rel[ENH_VEC_LENF - 1]);  */
	L_var_sum = L_shl(L_var_sum, 1);
	L_var_sum = L_sub(L_var_sum, L_deposit_l(var_rel[0]));
	L_var_sum = L_sub(L_var_sum, L_deposit_l(var_rel[ENH_VEC_LENF - 1]));
	var_rel_av = extract_l(L_shr(L_var_sum, 8));                       /* Q15 */

	/*	var_rel_av = (var_rel_av < 0 ? 0 : var_rel_av); */
	if (var_rel_av < 0)
		var_rel_av = 0;

	var_rel_av_sqrt = mult(X15_Q13, sqrt_Q15(var_rel_av));			   /* Q13 */
	var_rel_av_sqrt = add(ONE_Q13, var_rel_av_sqrt);                   /* Q13 */

	/*	tmp1 = var_rel_av_sqrt * Fvar; */
	/*	tmp2 = var_rel_av_sqrt * Fvar_sub; */
	tmp1 = extract_h(L_shl(L_mult(var_rel_av_sqrt, FVAR), 1));         /* Q10 */
	tmp2 = extract_h(L_shl(L_mult(var_rel_av_sqrt, FVAR_SUB), 1));     /* Q13 */

	L_max1 = 0;                                         /* for biased_spect[] */
	L_max2 = 0;                                     /* for biased_spect_sub[] */

	for (i = 0; i < ENH_VEC_LENF; i++){
		L_tmp3 = L_mult(var_rel_av_sqrt, smoothedspect[i]);            /* Q29 */
		tmp5 = var_rel[i];                                             /* Q15 */
		L_tmp4 = L_mult(tmp5, smoothedspect[i]);                       /* Q31 */

		/* quadratic approximation */
		tmp = add(MINV, shr(tmp5, 1));                                 /* Q14 */
		tmp = add(MINV2, shr(mult(tmp5, tmp), 1));                     /* Q13 */
		L_sum = L_mpy_ls(L_tmp4, tmp1);                                /* Q26 */
		L_sum = L_mpy_ls(L_sum, tmp);                                  /* Q24 */
		L_sum = L_add(L_shr(L_tmp3, 6), L_shr(L_sum, 1));              /* Q23 */
		if (L_sum < 1)
			L_sum = 1;
		shift1 = norm_l(L_sum);
		biased_spect[i] = extract_h(L_shl(L_sum, shift1));
		bias_shift[i] = add(sm_shift[i], sub(8, shift1));

		tmp = add(MINV_SUB, shr(tmp5, 2));                             /* Q13 */
		tmp = add(MINV_SUB2, shr(mult(tmp5, tmp), 1));                 /* Q12 */
		L_sum = L_mpy_ls(L_tmp4, tmp2);                                /* Q29 */
		L_sum = L_mpy_ls(L_sum, tmp);                                  /* Q26 */
		L_sum = L_add(L_shr(L_tmp3, 4), L_shr(L_sum, 1));              /* Q25 */
		if (L_sum < 1)
			L_sum = 1;
		shift1 = norm_l(L_sum);
		biased_spect_sub[i] = extract_h(L_shl(L_sum, shift1));
		bias_sub_shift[i] = add(sm_shift[i], sub(6, shift1));
	}
}


/***************************************************************************/
/*	Subroutine noise_slope: compute maximum of the permitted increase of   */
/*		   the noise estimate as a function of the mean signal variance    */
/***************************************************************************/
static Shortword	noise_slope(void)
{
	Shortword	noise_slope_max;


	if (var_rel_av > X018_Q15)
		noise_slope_max = X132_Q11;
	else if ((var_rel_av < X003_Q15) || (enh_i < 50))
		noise_slope_max = X88_Q11;
	else if (var_rel_av < X005_Q15)
		noise_slope_max = X44_Q11;
	else if (var_rel_av < X006_Q15)
		noise_slope_max = X22_Q11;
	else
		noise_slope_max  = X132_Q11;

	return(noise_slope_max);
}


/***************************************************************************/
/* Subroutine comp_data_shift: compare two block floating-point numbers.   */
/* It actually compares x1 = (num1 * 2^shift1) and x2 = (num2 * 2^shift2). */
/* The sign of the returned value is the same as that of (x1 - x2).        */
/***************************************************************************/
static Shortword	comp_data_shift(Shortword num1, Shortword shift1,
									Shortword num2, Shortword shift2)
{
	Shortword	shift;


	if ((num1 > 0) && (num2 < 0))
		return(1);
	else if ((num1 < 0) && (num2 > 0))
		return(-1);
	else {
		shift = sub(shift1, shift2);
		if (shift > 0)
			num2 = shr(num2, shift);
		else
			num1 = shl(num1, shift);

		return(sub(num1, num2));
	}
}


/***************************************************************************/
/*	Subroutine min_search: find minimum of psd's in circular buffer 	   */
/***************************************************************************/
static void		min_search(Shortword biased_spect[], Shortword bias_shift[],
						   Shortword biased_spect_sub[],
						   Shortword bias_sub_shift[])
{
	register Shortword	i, k;
	static BOOLEAN	localflag[ENH_VEC_LENF];       /* local minimum indicator */
	static Shortword	minspec_counter = 0;             /* count sub-windows */
	static Shortword	circb_index = 0;               /* ring buffer counter */
	static Shortword	circb_min[ENH_VEC_LENF];
                                                /* minimum of circular buffer */
    static Shortword	circb_min_shift[ENH_VEC_LENF];
	Shortword	noise_slope_max, tmp, tmp_shift, temp1, temp2;
	Longword	L_sum;


	/* localflag[] is initialized to FALSE since it is static. */

	if (minspec_counter == 0){
		noise_slope_max = noise_slope();

		for (i = 0; i < ENH_VEC_LENF; i++){
			if (comp_data_shift(biased_spect[i], bias_shift[i],
								act_min[i], act_min_shift[i]) < 0){
				act_min[i] = biased_spect[i];       /* update minimum */
				act_min_shift[i] = bias_shift[i];
				act_min_sub[i] = biased_spect_sub[i];
				act_min_sub_shift[i] = bias_sub_shift[i];
				localflag[i] = FALSE;
			}
		}

		/* write new minimum into ring buffer */
		v_equ(circb[circb_index], act_min, ENH_VEC_LENF);
		v_equ(circb_shift[circb_index], act_min_shift, ENH_VEC_LENF);

		for (i = 0; i < ENH_VEC_LENF; i++){
			/* Find minimum of ring buffer.  Using temp1 and temp2 as cache   */
			/* for circb_min[i] and circb_min_shift[i].                       */

			temp1 = circb[0][i];
			temp2 = circb_shift[0][i];
			for (k = 1; k < NUM_MINWIN; k++){
				if (comp_data_shift(circb[k][i], circb_shift[k][i], temp1,
									temp2) < 0){
					temp1 = circb[k][i];
					temp2 = circb_shift[k][i];
				}
			}
			circb_min[i] = temp1;
			circb_min_shift[i] = temp2;
		}
		for (i = 0; i < ENH_VEC_LENF; i++){
			/*	rapid update in case of local minima which do not deviate
				more than noise_slope_max from the current minima */
			tmp = mult(noise_slope_max, circb_min[i]);
			tmp_shift = add(circb_min_shift[i], 4);         /* adjust for Q11 */
			if (localflag[i] &&
				comp_data_shift(act_min_sub[i], act_min_sub_shift[i],
								circb_min[i], circb_min_shift[i]) > 0 &&
				comp_data_shift(act_min_sub[i], act_min_sub_shift[i], tmp,
								tmp_shift) < 0){
				circb_min[i] = act_min_sub[i];
				circb_min_shift[i] = act_min_sub_shift[i];

				/* propagate new rapid update minimum into ring buffer */
				for (k = 0; k < NUM_MINWIN; k++){
					circb[k][i] = circb_min[i];
					circb_shift[k][i] = circb_min_shift[i];
				}
			}
		}

		/* reset local minimum indicator */
		fill(localflag, FALSE, ENH_VEC_LENF);

		/* increment ring buffer pointer */
		circb_index = add(circb_index, 1);
		if (circb_index == NUM_MINWIN)
			circb_index = 0;
	} else if (minspec_counter == 1){

		v_equ(act_min, biased_spect, ENH_VEC_LENF);
		v_equ(act_min_shift, bias_shift, ENH_VEC_LENF);
		v_equ(act_min_sub, biased_spect_sub, ENH_VEC_LENF);
		v_equ(act_min_sub_shift, bias_sub_shift, ENH_VEC_LENF);

	} else {                                           /* minspec_counter > 1 */

		/* At this point localflag[] is all FALSE.  As we loop through        */
		/* minspec_counter, if any localflag[] is turned TRUE, it will be     */
		/* preserved until we go through the (minspec_counter == 0) branch.   */

		for (i = 0; i < ENH_VEC_LENF; i++){
			if (comp_data_shift(biased_spect[i], bias_shift[i],
								act_min[i], act_min_shift[i]) < 0){
				/* update minimum */
				act_min[i] = biased_spect[i];
				act_min_shift[i] = bias_shift[i];
				act_min_sub[i] = biased_spect_sub[i];
				act_min_sub_shift[i] = bias_sub_shift[i];
				localflag[i] = TRUE;
			}
		}
		for (i = 0; i < ENH_VEC_LENF; i++){
			if (comp_data_shift(act_min_sub[i], act_min_sub_shift[i],
								circb_min[i], circb_min_shift[i]) < 0){
				circb_min[i] = act_min_sub[i];
				circb_min_shift[i] = act_min_sub_shift[i];
			}
		}

		v_equ(noisespect, circb_min, ENH_VEC_LENF);
		v_equ(noise_shift, circb_min_shift, ENH_VEC_LENF);

		for (i = 0; i < ENH_VEC_LENF; i++){
			L_sum = L_mult(ENH_NOISE_BIAS, noisespect[i]);
			if (L_sum < 0x40000000L){
				L_sum = L_shl(L_sum, 1);
				lambdaD_shift[i] = noise_shift[i];
			} else
				lambdaD_shift[i] = add(noise_shift[i], 1);
			lambdaD[i] = extract_h(L_sum);
		}
	}
	minspec_counter = add(minspec_counter, 1);
	if (minspec_counter == LEN_MINWIN)
		minspec_counter = 0;
}


/***************************************************************************/
/*	Subroutine enh_init: initialization of variables for the enhancement   */
/***************************************************************************/
void enh_init()
{
	register Shortword	i;
	Shortword	max, shift, temp, norm_shift, guard;
	Shortword	a_shift;
	Longword	L_sum = 0, L_data;


	/* initialize noise spectrum */
	
	/* Because initial_noise[] is read once and then never used, we can use   */
	/* it for tempV2[] too.                                                   */

	window_M(initial_noise, sqrt_tukey_256_180, initial_noise, ENH_WINLEN);

	/* ---- Find normalization factor of the input signal ---- */
	/* Starting max with 1 to avoid norm_s(0). */
	max = 1;
	for (i = 0; i < ENH_WINLEN; i++){
		temp = abs_s(initial_noise[i]);
		if (temp > max)
			max = temp;
	}
	shift = norm_s(max);

	/* Now remember the amount of shift, normalize initial_noise[], then      */
	/* process initial_noise[] as Q15. */
	a_shift = sub(15, shift);

	/* transfer to frequency domain */
	v_zap(ybuf, 2*ENH_WINLEN + 2);
	for (i = 0; i < ENH_WINLEN; i++)
		ybuf[2*i] = shl(initial_noise[i], shift);

	guard = fft_npp(ybuf, 1);
	/* guard = fft(ybuf, ENH_WINLEN, MONE_Q15); */

	/* get rough estimate of lambdaD  */
	temp_yy[0] = L_shr(L_mult(ybuf[0], ybuf[0]), 1);
	temp_yy[1] = 0;
	temp_yy[ENH_WINLEN] = L_shr(L_mult(ybuf[ENH_WINLEN], ybuf[ENH_WINLEN]), 1);
	temp_yy[ENH_WINLEN + 1] = 0;

	for (i = 2; i < ENH_WINLEN - 1; i += 2){
		temp_yy[i + 1] = 0;
		temp_yy[i] = L_shr(L_add(L_mult(ybuf[i], ybuf[i]),
								 L_mult(ybuf[i + 1], ybuf[i + 1])), 1);
	}

	L_sum = temp_yy[0];
	for (i = 1; i < ENH_WINLEN + 1; i++){
		if (L_sum < temp_yy[i])
			L_sum = temp_yy[i];
	}
	shift = norm_l(L_sum);
	for (i = 0; i < ENH_WINLEN + 1; i++)
		ybuf[i] = extract_h(L_shl(temp_yy[i], shift));
	shift = sub(shl(add(a_shift, guard), 1), add(shift, 7));

	/* convert to correlation domain */
	for (i = 0; i < ENH_WINLEN/2 - 1; i++){
		ybuf[ENH_WINLEN + 2*i + 2] = ybuf[ENH_WINLEN - 2*i - 2];
		ybuf[ENH_WINLEN + 2*i + 3] = negate(ybuf[ENH_WINLEN - 2*i - 2 + 1]);
	}
	guard = fft_npp(ybuf, -1);                    /* inverse fft */
	/* guard = fft(ybuf, ENH_WINLEN, ONE_Q15); */
	shift = add(shift, guard);
	shift = sub(shift, 8);
	for (i = 0; i < ENH_WINLEN; i++)
		initial_noise[i] = ybuf[2*i];

	max = 0;
	for (i = 0; i < ENH_WINLEN; i++){
		temp = abs_s(initial_noise[i]);
		if (temp > max)
			max = temp;
	}
	temp = norm_s(max);
	shift = sub(shift, temp);
	for (i = 0; i < ENH_WINLEN; i++)
		initial_noise[i] = shl(initial_noise[i], temp);

	smoothing_win(initial_noise);

	/* convert back to frequency domain */
	v_zap(ybuf, 2*ENH_WINLEN + 2);
	for (i = 0; i < ENH_WINLEN; i++)
		ybuf[2*i] = initial_noise[i];
	guard = fft_npp(ybuf, 1);
	/* guard = fft(ybuf, ENH_WINLEN, MONE_Q15); */
	for (i = 0; i <= (ENH_WINLEN*2); i += 2){
		if (ybuf[i] < 0)
			ybuf[i] = 0;
	}
	norm_shift = add(shift, guard);

	/* rough estimate of lambdaD */
	L_sum = L_shl(L_mult(NOISE_B_BY_NSMTH_B, ybuf[0]), 7);
	L_sum = L_add(L_sum, 2);
	shift = norm_l(L_sum);
	lambdaD[0] = extract_h(L_shl(L_sum, shift));
	lambdaD_shift[0] = add(norm_shift, sub(1, shift));
	L_sum = L_shr(L_sum, 8);

	L_data = L_shl(L_mult(NOISE_B_BY_NSMTH_B, ybuf[ENH_WINLEN]), 7);
	L_data = L_add(L_data, 2);
	shift = norm_l(L_data);
	lambdaD[ENH_WINLEN/2] = extract_h(L_shl(L_data, shift));
	lambdaD_shift[ENH_WINLEN/2] = add(norm_shift, sub(1, shift));
	L_sum = L_add(L_sum, L_shr(L_data, 8));

	for (i = 1; i < ENH_WINLEN/2; i++){
		L_data = L_shl(L_mult(NOISE_B_BY_NSMTH_B, ybuf[2*i]), 7);
		L_data = L_add(L_data, 2);
		shift = norm_l(L_data);
		lambdaD[i] = extract_h(L_shl(L_data, shift));
		lambdaD_shift[i] = add(norm_shift, sub(1, shift));
		L_sum = L_add(L_sum, L_shr(L_data, 7));
	}

	shift = norm_l(L_sum);
	n_pwr = extract_h(L_shl(L_sum, shift));
	n_pwr_shift = sub(add(norm_shift, 1), shift);

	/* compute initial long term SNR; speech signal power depends on
	   the window; the Hanning window is used as a reference here with
	   a squared norm of 96 Q4 */

	temp = 14648;                                              /* 0.894/2 Q15 */
	shift = 22;                                    /* exp = 22, sum = 1875000 */
	SN_LT = divide_s(temp, n_pwr);
	SN_LT_shift = sub(shift, n_pwr_shift);

	minstat_init();
}


/***************************************************************************/
/*	Subroutine minstat_init: initialization of variables for minimum	   */
/*							 statistics noise estimation				   */
/***************************************************************************/
static void		minstat_init(void)
{
	/* Initialize Minimum Statistics Noise Estimator */
	register Shortword	i;
	Shortword	shift;
	Longword	L_sum;


	v_equ(smoothedspect, lambdaD, ENH_VEC_LENF);
	v_scale(smoothedspect, ENH_INV_NOISE_BIAS, ENH_VEC_LENF);

	for (i = 0; i < NUM_MINWIN; i++){
		v_equ(circb[i], smoothedspect, ENH_VEC_LENF);
		v_equ(circb_shift[i], lambdaD_shift, ENH_VEC_LENF);
	}

	v_equ(sm_shift, lambdaD_shift, ENH_VEC_LENF);
	v_equ(act_min, smoothedspect, ENH_VEC_LENF);
	v_equ(act_min_shift, lambdaD_shift, ENH_VEC_LENF);
	v_equ(act_min_sub, smoothedspect, ENH_VEC_LENF);
	v_equ(act_min_sub_shift, lambdaD_shift, ENH_VEC_LENF);
	v_equ(noisespect, smoothedspect, ENH_VEC_LENF);
	v_equ(noise_shift, lambdaD_shift, ENH_VEC_LENF);

	for (i = 0; i < ENH_VEC_LENF; i++){
		var_sp_av[i] = mult(smoothedspect[i], 20066);       /* Q14, sqrt(3/2) */
		av_shift[i] = add(lambdaD_shift[i], 1);
		L_sum = L_mult(smoothedspect[i], smoothedspect[i]);
		shift = norm_l(L_sum);
		var_sp_2[i] = extract_h(L_shl(L_sum, shift));
		av2_shift[i] = sub(shl(lambdaD_shift[i], 1), sub(shift, 1));
	}

	alphacorr = X09_Q15;
}


/***************************************************************************/
/* Subroutine:		process_frame
**
** Description:     Enhance a given frame of speech
**
** Arguments:
**
**  Shortword   inspeech[]  ---- input speech data buffer (Q0)
**  Shortword   outspeech[] ---- output speech data buffer (Q0)
**
** Return value:    None
**
 ***************************************************************************/
static void		process_frame(Shortword inspeech[], Shortword outspeech[])
{
	register Shortword	i;
	static BOOLEAN	first_time = TRUE;
	static Shortword	agal[ENH_VEC_LENF];                     /* GainD*Ymag */
	static Shortword	agal_shift[ENH_VEC_LENF];
	static Shortword	Ksi_min_var = GM_MIN;
	static Shortword	qk[ENH_VEC_LENF];   /* probability of noise only, Q15 */
	static Shortword	Gain[ENH_VEC_LENF];    /* MMSE LOG STA estimator gain */
	static Shortword	YY_LT, YY_LT_shift;
	static Shortword	SN_LT0, SN_LT0_shift;
	BOOLEAN		n_flag;
	Shortword	temp, temp_shift;
	Shortword	YY_av, gamma_av, gamma_max, gamma_av_shift, gamma_max_shift;
	Shortword	shift, YY_av_shift, max, temp1, temp2, temp3, temp4;
	Shortword	max_YY_shift, Y_shift, guard;
	Shortword	a_shift, ygal;
	Shortword	Ymag[ENH_VEC_LENF];             /* magnitude of current frame */
	Shortword	Ymag_shift[ENH_VEC_LENF];
	Shortword	GainD[ENH_VEC_LENF];                          /* Gain[].*GM[] */
	Shortword	analy[ENH_WINLEN];
	Shortword	gamaK[ENH_VEC_LENF];                      /* a posteriori SNR */
    Shortword	gamaK_shift[ENH_VEC_LENF];
	Shortword	biased_smoothedspect[ENH_VEC_LENF];      /* weighted smoothed */
                                                    /* spect. for long window */
    Shortword	biased_smoothedspect_sub[ENH_VEC_LENF];      /* for subwindow */
    Shortword	bias_shift[ENH_VEC_LENF], bias_sub_shift[ENH_VEC_LENF];
	Longword	L_sum, L_max;


	if (first_time){
		v_zap(agal, ENH_VEC_LENF);
		v_zap(agal_shift, ENH_VEC_LENF);
		fill(ksi, GM_MIN, ENH_VEC_LENF);
		v_zap(ksi_shift, ENH_VEC_LENF);
		fill(qk, ENH_QK_MAX, ENH_VEC_LENF);
		fill(Gain, GM_MIN, ENH_VEC_LENF);
		fill(GainD, GM_MIN, ENH_VEC_LENF);
		YY_LT = 0;
		YY_LT_shift = 0;
		SN_LT0 = SN_LT;
		SN_LT0_shift = SN_LT_shift;

		first_time = FALSE;
	}

	if (enh_i < 50)                  /* This ensures enh_i does not overflow. */
		enh_i ++;

	/* analysis window */
	window_M(inspeech, sqrt_tukey_256_180, analy, ENH_WINLEN);    /* analy[] Q0 */

	/* ---- Find normalization factor of the input signal ---- */
	max = 1;                           /* max starts with 1 to avoid problems */
                                                 /* with shift = norm_s(max). */
	for (i = 0; i < ENH_WINLEN; i++){
		temp1 = abs_s(analy[i]);
		if (temp1 > max)
			max = temp1;
	}
	shift = norm_s(max);
	/* ---- Now think input is Q15 ---- */
	a_shift = sub(15, shift);

	/* ------------- Fixed point fft -------------- */
	/* into frequency domain - D->Y, D->YD->Y, YY_av,
	   real and imaginary parts are interleaved */
	for (i = 0; i < ENH_WINLEN; i++)
		analy[i] = shl(analy[i], shift);

	v_zap(ybuf, 2*ENH_WINLEN + 2);
	for (i = 0; i < 2*ENH_WINLEN; i += 2)
		ybuf[i] = analy[i/2];
	guard = fft_npp(ybuf, 1);
	/* guard = fft(ybuf, ENH_WINLEN, MONE_Q15); */

	/* Previously here we copy ybuf[] to Y[] and process Y[].  In fact this   */
	/* is not necessary because Y[] is not changed before we use ybuf[] and   */
	/* update ybuf[] the next time.                                           */

	/* ---- D->YY shift is compared to original data ---- */
	Y_shift = add(a_shift, guard);
	YY_av_shift = shl(Y_shift, 1);

	temp_yy[0] = L_mult(ybuf[0], ybuf[0]);
	temp_yy[ENH_VEC_LENF - 1] = L_mult(ybuf[ENH_WINLEN], ybuf[ENH_WINLEN]);

	for (i = 1; i < ENH_VEC_LENF - 1; i++)
		temp_yy[i] = L_add(L_mult(ybuf[2*i], ybuf[2*i]),
						   L_mult(ybuf[2*i + 1], ybuf[2*i + 1]));

	max_YY_shift = SW_MIN;
	for (i = 0; i < ENH_VEC_LENF; i++){
		if (temp_yy[i] < 1)
			temp_yy[i] = 1;
		shift = norm_l(temp_yy[i]);
		YY[i] = extract_h(L_shl(temp_yy[i], shift));
		YY_shift[i] = sub(YY_av_shift, shift);
		if (max_YY_shift < YY_shift[i])
			max_YY_shift = YY_shift[i];
	}

	for (i = 0; i < ENH_VEC_LENF; i++){
		temp = YY[i];
		temp_shift = YY_shift[i];
		if (temp_shift & 0x0001){
			temp = shr(temp, 1);
			temp_shift = add(temp_shift, 1);
		}
		Ymag[i] = sqrt_Q15(temp);
		Ymag_shift[i] = shr(temp_shift, 1);
		YY_shift[i] = sub(YY_shift[i], 8);
	}

	/* ---- Compute YY_av ---- */
	L_sum = L_shl(L_deposit_l(YY[0]), sub(7, sub(max_YY_shift, YY_shift[0])));
	L_sum = L_add(L_sum, L_shl(L_deposit_l(YY[ENH_VEC_LENF - 1]),
				  sub(7, sub(max_YY_shift, YY_shift[ENH_VEC_LENF - 1]))));
	for (i = 1; i < ENH_VEC_LENF - 1; i++)
		L_sum = L_add(L_sum, L_shl(L_deposit_l(YY[i]),
					  sub(8, sub(max_YY_shift, YY_shift[i]))));
	if (L_sum == 0)
		L_sum = 1;
	temp1 = norm_l(L_sum);
	YY_av = extract_h(L_shl(L_sum, temp1));
	YY_av_shift = sub(add(max_YY_shift, 1), temp1);

	/* compute smoothed short time periodogram */
	smoothed_periodogram(YY_av, YY_av_shift);

	/* compute inverse bias and multiply short time periodogram with inverse  */
	/* bias */
	bias_compensation(biased_smoothedspect, bias_shift,
					  biased_smoothedspect_sub, bias_sub_shift);

	/* determine unbiased noise psd estimate by minimum search */
	min_search(biased_smoothedspect, bias_shift, biased_smoothedspect_sub,
			   bias_sub_shift);

	/* compute 'gammas' */
	for (i = 0; i < ENH_VEC_LENF; i++){
		gamaK[i] = divide_s(shr(YY[i], 1), lambdaD[i]);
		gamaK_shift[i] = sub(add(YY_shift[i], 1), lambdaD_shift[i]);
	}

	/* compute average of gammas */
	L_sum = L_shl(L_deposit_l(gamaK[0]), 7);
	shift = sub(gamaK_shift[0], 1);
	for (i = 1; i < ENH_VEC_LENF - 1; i++){
		temp1 = sub(shift, gamaK_shift[i]);
		if (temp1 > 0)
			L_sum = L_add(L_sum, L_shr(L_deposit_l(gamaK[i]), sub(temp1, 7)));
		else{
			L_sum = L_add(L_shl(L_sum, temp1),
						  L_shl(L_deposit_l(gamaK[i]), 7));
			shift = gamaK_shift[i];
		}
	}
	temp1 = sub(shift, sub(gamaK_shift[ENH_VEC_LENF - 1], 1));
	if (temp1 > 0)
		L_sum = L_add(L_sum, L_shr(L_deposit_l(gamaK[ENH_VEC_LENF - 1]),
								   sub(temp1, 7)));
	else{
		L_sum = L_add(L_shl(L_sum, temp1),
					  L_shl(L_deposit_l(gamaK[ENH_VEC_LENF - 1]), 7));
		shift = sub(gamaK_shift[ENH_VEC_LENF - 1], 1);
	}
	if (L_sum == 0)
		L_sum = 1;
	temp1 = norm_l(L_sum);
	gamma_av = extract_h(L_shl(L_sum, temp1));
	gamma_av_shift = add(sub(shift, temp1), 10 - 8);

	gamma_max = gamaK[0];
	gamma_max_shift = gamaK_shift[0];
	for (i = 1; i < ENH_VEC_LENF; i++){
		if (comp_data_shift(gamma_max, gamma_max_shift, gamaK[i],
							gamaK_shift[i]) < 0){
			gamma_max = gamaK[i];
			gamma_max_shift = gamaK_shift[i];
		}
	}

	/* determine signal presence */
	n_flag = FALSE;                          /* default flag - signal present */

	if ((comp_data_shift(gamma_max, gamma_max_shift, GAMMAX_THR, 6) < 0) &&
		(comp_data_shift(gamma_av, gamma_av_shift, GAMMAV_THR, 1) < 0)){
		n_flag = TRUE;                                          /* noise-only */

		temp1 = mult(n_pwr, GAMMAV_THR);
		temp2 = add(n_pwr_shift, 1 + 1);
		if (comp_data_shift(YY_av, YY_av_shift, temp1, temp2) > 0)
			n_flag = FALSE;            /* overiding if frame SNR > 3dB (9/98) */
	}

	if (enh_i == 1){            /* Initial estimation of apriori SNR and Gain */
		n_flag = TRUE;

		for (i = 0; i < ENH_VEC_LENF; i++){
			temp_yy[i] = L_mult(Ymag[i], GM_MIN);
			shift = norm_l(temp_yy[i]);
			agal[i] = extract_h(L_shl(temp_yy[i], shift));
			agal_shift[i] = sub(Ymag_shift[i], shift);
		}
	} else {                                                     /* enh_i > 1 */

		/* estimation of apriori SNR */
		for (i = 0; i < ENH_VEC_LENF; i++){
			L_sum = L_mult(agal[i], agal[i]);
			L_sum = L_mpy_ls(L_sum, ENH_ALPHAK);
			if (L_sum < 1)
				L_sum = 1;
			shift = norm_l(L_sum);
			temp1 = extract_h(L_shl(L_sum, shift));
			temp2 = sub(shl(agal_shift[i], 1), add(shift, 8));
			temp3 = lambdaD[i];
			temp4 = lambdaD_shift[i];
			if (sub(temp3, temp1) < 0){
				temp1 = shr(temp1, 1);
				temp2 = (Shortword) (temp2 + 1);
			}
			ksi[i] = divide_s(temp1, temp3);
			ksi_shift[i] = sub(temp2, temp4);
			if (comp_data_shift(gamaK[i], gamaK_shift[i], ENH_INV_NOISE_BIAS,
								0) > 0){
				L_sum = L_shr(L_deposit_h(ENH_INV_NOISE_BIAS),
							  gamaK_shift[i]);
				L_sum = L_sub(L_deposit_h(gamaK[i]), L_sum);
				shift = norm_l(L_sum);
				temp1 = extract_h(L_shl(L_sum, shift));
				temp1 = mult(temp1, ENH_BETAK);      /* note ENH_BETAK is Q18 */
				temp2 = sub(gamaK_shift[i], add(shift, 3));
				shift = sub(ksi_shift[i], temp2);
				if (shift > 0){
					ksi[i] = add(shr(ksi[i], 1), shr(temp1, (Shortword) (shift + 1)));
					ksi_shift[i] = add(ksi_shift[i], 1);
				} else {
					ksi[i] = add(shl(ksi[i], (Shortword) (shift - 1)), shr(temp1, 1));
					ksi_shift[i] = add(temp2, 1);
				}
			}
		}

		/*	Ksi_min_var = 0.9*Ksi_min_var +
						  0.1*ksi_min_adapt(n_flag, ksi_min, SN_LT);          */
		temp1 = mult(X09_Q15, Ksi_min_var);
		temp2 = mult(X01_Q15, ksi_min_adapt(n_flag, GM_MIN, SN_LT,
					 SN_LT_shift));
		Ksi_min_var = add(temp1, temp2);

		shift = norm_s(Ksi_min_var);
		temp1 = shl(Ksi_min_var, shift);
		/* ---- limit the bottom of the ksi ---- */
		for (i = 0; i < ENH_VEC_LENF; i++){
			if (comp_data_shift(ksi[i], ksi_shift[i],
								temp1, negate(shift)) < 0){
				ksi[i] = temp1;
				ksi_shift[i] = negate(shift);
			}
		}

		/* estimation of k-th component 'signal absence' prob and gain */
		/* default value for qk's % (9/98)	*/
		fill(qk, ENH_QK_MAX, ENH_VEC_LENF);

		if (!n_flag){                                       /* SIGNAL PRESENT */
			/* computation of the long term SNR */
			if (comp_data_shift(gamma_av, gamma_av_shift, GAMMAV_THR, 1) > 0){

				/*	YY_LT = YY_LT * alpha_LT + beta_LT * YY_av; */
				L_sum = L_mult(YY_LT, ENH_ALPHA_LT);
				shift = norm_l(L_sum);
				temp1 = extract_h(L_shl(L_sum, shift));
				temp2 = sub(YY_LT_shift, shift);
				L_sum = L_mult(YY_av, ENH_BETA_LT);
				shift = norm_l(L_sum);
				temp3 = extract_h(L_shl(L_sum, shift));
				temp4 = sub(YY_av_shift, shift);
				temp1 = shr(temp1, 1);
				temp3 = shr(temp3, 1);
				shift = sub(temp2, temp4);
				if (shift > 0){
					YY_LT = add(temp1, shr(temp3, shift));
					YY_LT_shift = temp2;
				} else {
					YY_LT = add(shl(temp1, shift), temp3);
					YY_LT_shift = temp4;
				}
				YY_LT_shift = add(YY_LT_shift, 1);

				/*	SN_LT = (YY_LT/n_pwr) - 1;  Long-term S/N */
				temp1 = sub(YY_LT, n_pwr);
				if (temp1 > 0){
					YY_LT = shr(YY_LT, 1);
					YY_LT_shift = add(YY_LT_shift, 1);
				}
				SN_LT = divide_s(YY_LT, n_pwr);
				SN_LT_shift = sub(YY_LT_shift, n_pwr_shift);

				/*	if (SN_LT < 0); we didn't subtract 1 from SN_LT yet.      */
				if (comp_data_shift(SN_LT, SN_LT_shift, ONE_Q15, 0) < 0){
					SN_LT = SN_LT0;
					SN_LT_shift = SN_LT0_shift;
				} else {
					temp1 = ONE_Q15;
					L_sum = L_sub(L_deposit_h(SN_LT),
									L_shr(L_deposit_h(temp1), SN_LT_shift));
					shift = norm_l(L_sum);
					SN_LT = extract_h(L_shl(L_sum, shift));
					SN_LT_shift = sub(SN_LT_shift, shift);
				}

				/*	SN_LT0 = SN_LT; */
				SN_LT0 = SN_LT;
				SN_LT0_shift = SN_LT_shift;
			}

			/* Estimating qk's using hard-decision approach (7/98) */
			compute_qk(qk, gamaK, gamaK_shift, ENH_GAMMAQ_THR);

			/*	vec_limit_top(qk, qk, qk_max, vec_lenf); */
			/*	vec_limit_bottom(qk, qk, qk_min, vec_lenf); */
			for (i = 0; i < ENH_VEC_LENF; i++){
				if (qk[i] > ENH_QK_MAX)
					qk[i] = ENH_QK_MAX;
				else if (qk[i] < ENH_QK_MIN)
					qk[i] = ENH_QK_MIN;
			}

		}

		gain_log_mmse(qk, Gain, gamaK, gamaK_shift, ENH_VEC_LENF);

		/* Previously the gain modification is done outside of gain_mod() and */
		/* now it is moved inside. */

		v_equ(GainD, Gain, ENH_VEC_LENF);
		gain_mod(qk, GainD, ENH_VEC_LENF);

		/*	vec_mult(Agal, GainD, Ymag, vec_lenf); */
		for (i = 0; i < ENH_VEC_LENF; i++){
			L_sum = L_mult(GainD[i], Ymag[i]);
			shift = norm_l(L_sum);
			agal[i] = extract_h(L_shl(L_sum, shift));
			agal_shift[i] = sub(Ymag_shift[i], shift);

		}

	}

	/* enhanced signal in frequency domain */
	/*	 (implement ygal = GainD .* Y)	 */
	for (i = 0; i < ENH_WINLEN + 2; i++)
		temp_yy[i] = L_mult(ybuf[i], GainD[i/2]);
	L_max = 0;
	for (i = 0; i < ENH_WINLEN + 2; i++)
		if (L_max < L_abs(temp_yy[i]))
			L_max = L_abs(temp_yy[i]);

	shift = norm_l(L_max);
	for (i = 0; i < ENH_WINLEN + 2; i++)
		ybuf[i] = extract_h(L_shl(temp_yy[i], shift));
	shift = sub(Y_shift, shift);

	/* transformation to time domain */
	for (i = 0; i < ENH_WINLEN/2 - 1; i++){
		ybuf[ENH_WINLEN + 2*i + 2] = ybuf[ENH_WINLEN - 2*i - 2];
		ybuf[ENH_WINLEN + 2*i + 3] = negate(ybuf[ENH_WINLEN - 2*i - 2 + 1]);
	}

	guard = fft_npp(ybuf, -1);
	/* guard = fft(ybuf, ENH_WINLEN, ONE_Q15); */
	shift = add(shift, guard);
	shift = sub(shift, 8);

#if USE_SYN_WINDOW
	for (i = 0; i < ENH_WINLEN; i++){
		ygal = shl(ybuf[2*i], sub(shift, 15));
		outspeech[i] = mult(ygal, sqrt_tukey_256_180[i]);
	}
#endif

	/* Misc. updates */
	max_YY_shift = SW_MIN;
	for (i = 0; i < ENH_VEC_LENF; i++){
		if (max_YY_shift < lambdaD_shift[i])
			max_YY_shift = lambdaD_shift[i];
	}

	L_sum = L_shl(L_deposit_l(lambdaD[0]),
				  sub(7, sub(max_YY_shift, lambdaD_shift[0])));
	L_sum = L_add(L_sum, L_shl(L_deposit_l(lambdaD[ENH_VEC_LENF - 1]),
				  sub(7, sub(max_YY_shift,
							 lambdaD_shift[ENH_VEC_LENF - 1]))));

	for (i = 1; i < ENH_VEC_LENF - 1; i++)
		L_sum = L_add(L_sum, L_shl(L_deposit_l(lambdaD[i]),
					  sub(8, sub(max_YY_shift, lambdaD_shift[i]))));
	if (L_sum == 0)
		L_sum = 1;
	shift = norm_l(L_sum);
	n_pwr = extract_h(L_shl(L_sum, shift));
	n_pwr_shift = add(sub(max_YY_shift, shift), 1);
}


//NPP initialization
void npp_ini(void)
{

	v_zap(hpspeech, IN_BEG + BLOCK);	/* speech[] is declared IN_BEG + BLOCK */

	/* Initialize wr_array and wi_array */
	fs_init();
        
}
