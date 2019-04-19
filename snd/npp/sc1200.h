/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

/*------------------------------------------------------------------*/
/*                                                                  */
/* File:        sc1200.h                                            */
/*                                                                  */
/* Description: globle include file for sc1200 coder                */
/*                                                                  */
/*------------------------------------------------------------------*/

#ifndef  _SC1200_H_
#define  _SC1200_H_


/* =================== */
/* Definition of Types */
/* =================== */

typedef long int		Longword;				/* 32 bit "accumulator" (L_*) */
typedef short int		Shortword;				/* 16 bit "register" (sw*) */
typedef short int		ShortwordRom;			/* 16 bit ROM data (sr*) */
typedef long int		LongwordRom;			/* 32 bit ROM data (L_r*) */
typedef unsigned short	UShortword;				/* 16 bit unsigned data */
typedef unsigned long	ULongword;				/* 32 bit unsigned data */
typedef double			Word40;					/* 40 bit "accumulator"	*/					
/* ================== */
/* General Definition */
/* ================== */

#define BOOLEAN		Shortword
#define TRUE		1
#define FALSE		0
#define ON			1
#define OFF			0

#ifndef PI
#define PI			3.141592654
#endif

/* ================= */
/* Software switches */
/* ================= */

#define ANA_SYN			0
#define ANALYSIS		1
#define SYNTHESIS		2
#define UP_TRANS		3
#define DOWN_TRANS		4

#define RATE2400		2400
#define RATE1200		1200

#define SKIP_CHANNEL    FALSE

#define POSTFILTER      TRUE
#define NPP             TRUE
#define	NEW_DC_FILTER	TRUE

/* ================================================================ *
 *                  Global Constants                                *
 * ================================================================ */

#define FSAMP               8000                        /* sampling frequency */
#define FRAME				180                          /* speech frame size */
#define NF					3                /* number of frames in one block */
#define BLOCK				(NF*FRAME)
#define FFTLENGTH			512
#define LOOKAHEAD			(FRAME + 110)

/* -------- Pitch analysis -------- */

#define PITCHMIN			20                           /* minimum pitch lag */
#define PITCHMAX			160                          /* maximum pitch lag */
#define PITCHMAX_Q7			20480                      /* PITCHMAX * (1 << 7) */
#define PITCHMIN_Q7			2560                       /* PITCHMIN * (1 << 7) */
#define MINPITCH			20                           /* new minimum pitch */
#define MAXPITCH			147                          /* new maximum pitch */
#define LOG_UV_PITCH_Q12	6963                           /* 1.7 * (1 << 12) */
#define NODE				8
#define NUM_PITCHES			2
#define PIT_COR_LEN			220
#define PIT_SUBNUM			2
#define PIT_SUBFRAME		(FRAME/PIT_SUBNUM)	/* 90 */

/* -------- Voicing analysis -------- */
/* In this coder, classy being TRANSITION is processed in the same way as     */
/* those classy being VOICED.                                                 */
#define VOICED				0                            /* new voicing modes */
#define UNVOICED			1               /* compatitable with MELP uv_flag */
#define TRANSITION			VOICED          /* here it is plosive in unvoiced */
#define SILENCE				3                /* silence is encoded same as uv */
#define NUM_BANDS			5                    /* number of frequency bands */
#define MINLENGTH			160                 /* minimum correlation length */
#define EN_FILTER_ORDER		17

/* -------- Buffer Structure -------- */
#define FRAME_BEG			(PITCHMAX - (FRAME/2))	/* 70 */
#define FRAME_END			(PITCHMAX + (FRAME/2))	/* 250 */
#define PITCH_BEG			(FRAME/2) /* 90 */
#define PITCH_FR			(2*PITCHMAX + 1) /* 321 */
#define IN_BEG				(PITCH_BEG + PITCHMAX + PIT_COR_LEN/2) /* 360 */
#define OLD_IN_BEG			(PITCH_BEG + PITCH_FR - FRAME) /* 231 */

/* -------- LPC analysis -------- */
#define LPC_FRAME			200                            /* LPC window size */
#define LPC_ORD				10                                   /* LPC order */
#define NUM_HARM			10                /* number of Fourier magnitudes */
#define NUM_GAINFR			2                    /* number of gains per frame */
#define PITCHMAX_X2			(PITCHMAX*2)         /* maximum pitch lag times 2 */
#define LPF_ORD				6                         /* lowpass filter order */
#if NEW_DC_FILTER
	#define	DC_ORD			6
#else
	#define DC_ORD			4                      /* DC removal filter order */
#endif
#define BPF_ORD				6               /* bandpass analysis filter order */
#define ENV_ORD				2               /* bandpass envelope filter order */
#define MIX_ORD				32            /* mixed excitation filtering order */
#define DISP_ORD			64               /* pulse dispersion filter order */
#define DEFAULT_PITCH_Q7	6400                       /* default pitch value */
                                                  /* DEFAULT_PITCH * (1 << 7) */
#define UV_PITCH			50                        /* unvoiced pitch value */
#define UV_PITCH_Q7			6400                       /* UV_PITCH * (1 << 7) */
#define VMIN_Q14			13107        /* 0.8 * (1 << 14)  minimum strongly */
                                                        /* voiced correlation */
#define VJIT_Q14			8192          /* 0.5 * (1 << 14) jitter threshold */
                                                          /* for correlations */
#define BPTHRESH_Q14		9830          /* 0.6 * (1 << 14) bandpass voicing */
                                                                 /* threshold */
#define MAX_JITTER_Q15		8192           /* 0.25 * (1 << 15) maximum jitter */
                                                           /* (as a fraction) */
#define ASE_NUM_BW_Q15		16384                 /* 0.5 * (1 << 15) adaptive */
                                                /* spectral enhancement-numer */
#define ASE_DEN_BW_Q15		26214        /* 0.8 * (1 << 15) adaptive spectral */
                                                         /* enhancement-denom */
#define GAINFR				(FRAME/NUM_GAINFR)          /* size of gain frame */
#define MIN_GAINFR			120               /* minimum gain analysis window */
#define GAIN_PITCH_Q7		15257     /* (1.33*GAINFR - 0.5) * (1 <<7 ) pitch */
                                                        /* input for gain_ana */
#define MSVQ_M				8                       /* M-best for MSVQ search */
#define MSVQ_MAXCNT			256 /* maximum inner loop counter for MSVQ search */

/* Correct value should be 409 but 0 was used during listening test. */
/* Set to 0 for bit exact results for the test materials */

#if (0)
#define BWMIN_Q15			0             /* (50*2/FSAMP) * (1 << 15) minimum */
                                                            /* LSF separation */
#else
#define BWMIN_Q15			409
#endif


/* ======================================= */
/* Noise suppression/estimation parameters */
/* Up by 3 dB/sec (0.5*22.5 ms frame),     */
/* down by 12 dB/sec                       */
/* ======================================= */

#define UPCONST_Q19		17691       /* 0.0337435 * (1 << 19) noise estimation */
                                                             /* up time const */
#define DOWNCONST_Q17	-17749      /* -0.135418 * (1 << 17) noise estimation */
                                                             /* dn time const */
#define NFACT_Q8		768         /* 3.0 * (1 << 8) noise floor boost in dB */
#define MAX_NS_ATT_Q8	1536      /* 6.0 * (1 << 8) maximum noise suppression */
#define MAX_NS_SUP_Q8	5120        /* 20.0 * (1 << 8) max noise level to use */
                                                            /* in suppression */
#define MIN_NOISE_Q8	2560             /* 10.0 * (1 << 8) min value allowed */
                                                       /* in noise estimation */
#define MAX_NOISE_Q8	20480            /* 80.0 * (1 << 8) max value allowed */
                                                       /* in noise estimation */


/* ===================== */
/* Channel I/O constants */
/* ===================== */

#define CHWORDSIZE		8                  /* number of bits per channel word */
#define ERASE_MASK		(UShortword) 0x4000              /* erasure flag mask */
                                                          /* for channel word */
#define GN_QLO_Q8		2560            /* 10.0 * (1 << 8) minimum gain in dB */
#define GN_QUP_Q8		19712           /* 77.0 * (1 << 8) maximum gain in dB */
#define GN_QLEV			32
#define GN_QLEV_M1		(32 - 1)       /* no. of 2nd gain quantization levels */
#define GN_QLEV_M1_Q10	31744     /* GN_QLEV_M1 * (1 << 10) GN_QLEV_M1 in Q10 */
#define PIT_BITS		7                  /* number of bits for pitch coding */
#define PIT_QLEV		99                  /* number of pitch levels minus 1 */
#define PIT_QLEV_M1		(99 - 1)            /* number of pitch levels minus 1 */
#define PIT_QLEV_M1_Q8	25088              /* (PIT_QLEV_M1*(1<<8))  number of */
                                                      /* pitch levels minus 1 */
#define PIT_QLO_Q12		5329               /* 1.30103 * (1 << 12) minimum log */
                                                    /* pitch for quantization */
#define PIT_QUP_Q12		9028               /* 2.20412 * (1 << 12) maximum log */
                                                    /* pitch for quantization */
#define FS_BITS			8            /* number of bits for Fourier magnitudes */
#define FS_LEVELS		(1 << FS_BITS)                /* number of levels for */
                                                        /* Fourier magnitudes */
#define CHSIZE			27                  /* The shortest integer number of */
                                             /* words in channel packet; NF*9 */
#define NUM_CH_BITS		81              /* the size is determined for 1.2kbps */


/* ===================== */
/* 1200 bps Quantization */
/* ===================== */
#define UV_BITS					3
#define DELTA_PITCH_WEIGHT_Q0	1                           /* 1.0 * (1 << 0) */
#define PITCH_VQ_CAND			16
#define PITCH_VQ_BITS			9
#define PITCH_VQ_SIZE			(1 << PITCH_VQ_BITS)
#define PITCH_VQ_LEVEL_VVV		(4 * PITCH_VQ_SIZE)
#define PITCH_VQ_LEVEL_UVV		PITCH_VQ_SIZE       /* This also includes VUV */
#define PITCH_VQ_LEVEL_VUV		PITCH_VQ_SIZE               /* and VVU cases. */
#define PITCH_VQ_LEVEL_VVU		PITCH_VQ_SIZE

#define GAIN_VQ_BITS			10
#define GAIN_VQ_SIZE			(1 << GAIN_VQ_BITS)

#define MAX_LSF_STAGE			4               /* We use 4 for VOICED frames */
                                                /* and 1 for UNVOICED frames. */
#define LSP_VQ_CAND				8
#define LSP_VQ_STAGES			4
#define INVALID_BPVC			0001

/* ================== */
/* Harmonic Synthesis */
/* ================== */

#define SYN_SUBFRAME		45
#define SYN_SUBNUM			(FRAME/SYN_SUBFRAME)


/* ===================== */
/* Structure definitions */
/* ===================== */

struct melp_param {                                        /* MELP parameters */
	Shortword	pitch;                                                  /* Q7 */
	Shortword	lsf[LPC_ORD];                                          /* Q15 */
	Shortword	gain[NUM_GAINFR];                                       /* Q8 */
	Shortword	jitter;                                                /* Q15 */
	Shortword	bpvc[NUM_BANDS];                                       /* Q14 */
	BOOLEAN		uv_flag;
	Shortword	fs_mag[NUM_HARM];                                      /* Q13 */
};

#define MSVQ_STAGES		4

struct quant_param {
	Shortword	pitch_index;                 /* quantized index of parameters */
	Shortword	lsf_index[NF][MAX_LSF_STAGE];
	Shortword	gain_index[NUM_GAINFR];
	Shortword	jit_index[NF];      /* Suspected that a scalar is good enough */
	Shortword	bpvc_index[NF];
	Shortword	fs_index;
	BOOLEAN		uv_flag[NF];                            /* global uv decision */
	Shortword	msvq_index[MSVQ_STAGES];
	Shortword	fsvq_index;
	unsigned char	*chptr;                     /* channel related parameters */
	Shortword	chbit;                             /* they are rate dependent */
};

void	analysis(Shortword sp_in[], struct melp_param *par);

void	sc_ana(struct melp_param *par);

void	synthesis(struct melp_param *par, Shortword sp_out[]);

void	melp_ana_init();

void	melp_syn_init();

void	melp_chn_write(struct quant_param *qpar, unsigned char chbuf[]);

void	low_rate_chn_write(struct quant_param *qpar);

BOOLEAN	melp_chn_read(struct quant_param *qpar, struct melp_param *par,
					  struct melp_param *prev_par, unsigned char chbuf[]);

BOOLEAN	low_rate_chn_read(struct quant_param *qpar, struct melp_param *par,
                          struct melp_param *prev_par);

void	fec_code(struct quant_param *par);

Shortword	fec_decode(struct quant_param *par, Shortword erase);


#endif
