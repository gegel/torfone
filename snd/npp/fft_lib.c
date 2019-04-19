/* ================================================================== */
/*                                                                    */ 
/*    Microsoft Speech coder     ANSI-C Source Code                   */
/*    SC1200 1200 bps speech coder                                    */
/*    Fixed Point Implementation      Version 7.0                     */
/*    Copyright (C) 2000, Microsoft Corp.                             */
/*    All rights reserved.                                            */
/*                                                                    */ 
/* ================================================================== */

/* ===================================== */
/* fft_lib.c: Fourier series subroutines */
/* ===================================== */

/* compiler include files */
#include "sc1200.h"
#include "mathhalf.h"
#include "mat_lib.h"
#include "math_lib.h"
#include "constant.h"
#include "global.h"
#include "fft_lib.h"

#define SWAP(a, b)		{Shortword temp1; temp1 = (a); (a) = (b); (b) = temp1;}

/* Memory definition */
static Shortword		wr_array[FFTLENGTH/2];
static Shortword		wi_array[FFTLENGTH/2];


/* Radix-2, DIT, 2N-point Real FFT */
void rfft(Shortword datam1[], Shortword n)
{
	Shortword	i, n_2;
	Shortword	r1, r2, i1, i2;
	Shortword	wr, wi, data_max;
	Shortword	index;
	Longword	L_temp1, L_temp2;

	n_2 = shr(n, 1);
	cfft(datam1, n_2);
	/* Check for overflow */
	data_max = 0;
	for (i = 0; i < n; i++){
		if (sub(data_max, abs_s(datam1[i])) < 0)
			data_max = abs_s(datam1[i]);
	}
	if (data_max > 16383){
		for (i = 0; i < n; i++)
			datam1[i] = shr(datam1[i], 1);
	}
	/* Checking ends */
	for (i = 2; i < n_2; i += 2){
		r1		= add_shr(datam1[i], datam1[n-i]);		/* Xr(k) + Xr(N-k):RP */
		L_temp1 = L_sub(datam1[i+1], datam1[n-i+1]);	/* Xi(k) - Xi(N-k):IM */
		L_temp1	= L_shl(L_temp1,16);
		r2		= add_shr(datam1[i+1], datam1[n-i+1]);	/* Xi(k) + Xi(N-k):IP */
		L_temp2	= L_sub(datam1[i], datam1[n-i]);		/* Xr(k) - Xr(N-k):RM */
		L_temp2	= L_shl(L_temp2, 16);
		datam1[i]		= r1;
		datam1[n-i]		= r1;
		datam1[2*n-i+1]	= extract_h(L_shr(L_temp2, 1));
		L_temp2			= L_negate(L_temp2);
		datam1[n+i+1]	= extract_h(L_shr(L_temp2, 1));

		datam1[i+1]		= r2;
		datam1[n-i+1]	= r2;
		datam1[2*n-i]	= extract_h(L_shr(L_temp1, 1));
		L_temp1			= L_negate(L_temp1);
		datam1[n+i]		= extract_h(L_shr(L_temp1, 1));
	}
	datam1[n+n_2]	= 0;
	datam1[n+n_2+1]	= 0;
	r1 = add(datam1[0], datam1[1]);
	i1 = sub(datam1[0], datam1[1]);
	datam1[0]	= r1;
	datam1[1]	= 0;
	datam1[n]	= i1;
	datam1[n+1]	= 0;
	index = 1;
	wr = wr_array[index];
	wi = wi_array[index];
	for (i = 2; i < n; i += 2){
		r1 = datam1[i];
		i1 = datam1[2*n-i];
		r2 = datam1[i+1];
		i2 = datam1[2*n-i+1];

		L_temp1 = L_deposit_h(r1);
		L_temp1	= L_add(L_temp1, L_mult(r2, wr));
		L_temp1 = L_add(L_temp1, L_shl(0x0080, 8));	
		L_temp1	= L_shl(L_shr(L_temp1,16), 16);		/* r_ound */
		L_temp1 = L_sub(L_temp1, L_mult(i2, wi));
		L_temp1 = L_add(L_temp1, L_shl(0x0080, 8));	/* r_ound */
		datam1[i]		= extract_h(L_temp1);
		datam1[2*n-i]	= extract_h(L_temp1);

		L_temp2 = L_deposit_h(i1);
		L_temp2 = L_sub(L_temp2, L_mult(r2, wi));
		L_temp2 = L_add(L_temp2, L_shl(0x0080, 8));	
		L_temp2	= L_shl(L_shr(L_temp2,16), 16);		/* r_ound */
		L_temp2 = L_sub(L_temp2, L_mult(i2, wr));
		L_temp2 = L_add(L_temp2, L_shl(0x0080, 8));	
		datam1[i+1]		= extract_h(L_temp2);
		datam1[2*n-i+1] = extract_h(L_negate(L_temp2));

		index += 1;
		wr = wr_array[index];
		wi = wi_array[index];
	}
}

/* Subroutine FFT: Fast Fourier Transform */
Shortword cfft(Shortword datam1[], Shortword nn)
{
	register Shortword	i, j;
	register Longword	L_tempr, L_tempi;
	Shortword	n, mmax, m, istep;
	Shortword	sPR, sQR, sPI, sQI;
	Shortword	wr, wi, data_max;
	Shortword	*data;
	Longword	L_temp1, L_temp2;
	Shortword	index, index_step;
	Shortword	guard;


	guard = 0;
	/* Use pointer indexed from 1 instead of 0 */
	data = &datam1[-1];

	n = shl(nn, 1);
	j = 1;
	for (i = 1; i < n; i += 2){
		if (j > i){
			SWAP(data[j], data[i]);
			SWAP(data[j + 1], data[i + 1]);
		}
		m = nn;
		while (m >= 2 && j > m){
			j = sub(j, m);
			m = shr(m, 1);
		}
		j = add(j, m);
	}

	/* Stage 1 */
	/* Check for Overflow */
	data_max = 0;
	for (i = 1; i <= n; i++){
		if (sub(data_max, abs_s(data[i])) < 0)
			data_max = abs_s(data[i]);
	}
	if (data_max > 16383){
		guard += 1;
		for (i = 1; i <= n; i++)
			data[i] = shr(data[i], 1);
	} 
	/* End of checking */
	for (i = 0; i < n; i += 4){
		sPR = datam1[i];
		sQR = datam1[i+2];
		sPI = datam1[i+1];
		sQI = datam1[i+2+1];
		datam1[i]		= add(sPR,sQR); /* PR' */
		datam1[i+2]		= sub(sPR,sQR); /* QR' */
		datam1[i+1]		= add(sPI,sQI); /* PI' */
		datam1[i+2+1]	= sub(sPI,sQI); /* QI' */
	}
	/* Stage 2 */
	/* Check for Overflow */
	data_max = 0;
	for (i = 1; i <= n; i++){
		if (sub(data_max, abs_s(data[i])) < 0)
			data_max = abs_s(data[i]);
	}
	if (data_max > 16383){
		guard += 1;
		for (i = 1; i <= n; i++)
			data[i] = shr(data[i], 1);
	} 
	/* End of checking */
	for (i = 0; i < n; i += 8){
		/* Butterfly 1 */
		sPR = datam1[i];
		sQR = datam1[i+4];
		sPI = datam1[i+1];
		sQI = datam1[i+4+1];
		datam1[i]		= add(sPR,sQR); /* PR' */
		datam1[i+4]		= sub(sPR,sQR); /* QR' */
		datam1[i+1]		= add(sPI,sQI); /* PI' */
		datam1[i+4+1]	= sub(sPI,sQI); /* QI' */
		/* Butterfly 2 */
		sPR = datam1[i+2];
		sQR = datam1[i+2+4];
		sPI = datam1[i+2+1];
		sQI = datam1[i+4+2+1];
		datam1[i+2]		= add(sPR,sQI); /* PR' */
		datam1[i+4+2]	= sub(sPR,sQI); /* QR' */
		datam1[i+2+1]	= sub(sPI,sQR); /* PI' */
		datam1[i+4+2+1]	= add(sPI,sQR); /* QI' */
	}
	/* Stages 3... */
	mmax = 8;

	/* initialize index step */
	index_step = shr(nn, 1);

	while (n > mmax){
		/* Overflow checking */
		data_max = 0;
		for (i = 1; i <= n; i++){
			if (sub(data_max, abs_s(data[i])) < 0)
				data_max = abs_s(data[i]);
		}
		if (data_max > 16383){
			guard += 2;
			for (i = 1; i <= n; i++)
				data[i] = shr(data[i], 2);
		} else if (data_max > 8191){
			guard += 1;
			for (i = 1; i <= n; i++)
				data[i] = shr(data[i], 1);
		}
		/* Checking ends */
		istep = shl(mmax, 1);                             /* istep = 2 * mmax */

		index = 0;
		index_step = shr(index_step, 1);

		wr = ONE_Q15;
		wi = 0;
		for (m = 1; m < mmax; m += 2){
			for (i = m; i <= n; i = add(i, istep)){
				j = add(i, mmax);

				L_temp1 = L_mult(wr, data[j]);
				L_temp2 = L_mult(wi, data[j+1]);
				L_tempr = L_add(L_temp1, L_temp2);
				L_tempr = L_add(L_tempr, L_shl(0x80, 8));
				L_tempr = L_shl(L_shr(L_tempr, 16), 16);

				sPR		= data[i];
				sQR		= data[j];
				data[i] = extract_h(L_add(L_deposit_h(sPR), L_tempr));
				data[j] = extract_h(L_sub(L_deposit_h(sPR), L_tempr));
				
				L_temp1 = L_mult(wi, sQR);
				L_temp2 = L_mult(wr, data[j+1]);
				L_tempi = L_sub(L_temp1, L_temp2);
				L_tempi = L_add(L_tempi, L_shl(0x80, 8));
				L_tempi = L_shl(L_shr(L_tempi,16), 16);

				sPI		= data[i+1];
				data[i+1] = extract_h(L_sub(L_deposit_h(sPI), L_tempi));
				data[j+1] = extract_h(L_add(L_deposit_h(sPI), L_tempi));
			}
			index = add(index, index_step);
			wr = wr_array[index];
			wi = wi_array[index];
		}
		mmax = istep;
	}
	return guard;
}


Shortword fft_npp(Shortword data[], Shortword dir)
{
	Shortword guard, n;
	
	guard = cfft(data, 256);
	if (dir < 0){ /* Reverse FFT */
		for (n = 1; n < 128; n++){
			SWAP(data[2*n], data[2*(256-n)]);
			SWAP(data[2*n+1], data[2*(256-n)+1]);
		}
	}
	return guard;
}


/* Initialization of wr_array and wi_array */
void fs_init(void)
{
	register Shortword	i;
	Shortword	fft_len2, shift, step, theta;


	fft_len2 = shr(FFTLENGTH, 1);
	shift = norm_s(fft_len2);
	step = shl(2, shift);
	theta = 0;

	for (i = 0; i < fft_len2; i++){
		wr_array[i] = cos_fxp(theta);
		wi_array[i] = sin_fxp(theta);
		if (i == (fft_len2 - 1))
			theta = ONE_Q15;
		else
			theta = add(theta, step);
	}
}
