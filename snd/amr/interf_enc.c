/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

/*
 * ===================================================================
 *  TS 26.104
 *  REL-5 V5.4.0 2004-03
 *  REL-6 V6.1.0 2004-03
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

/*
 * interf_enc.c
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    This module contains all the functions needed encoding 160
 *    16-bit speech samples to AMR encoder parameters.
 *
 */

/*
 * include files
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "amr_speech_importance.h"
#include "sp_enc.h"
#include "interf_rom.h"
#include "ophtools.h"

/*
 * Declare structure types
 */
/* Declaration transmitted frame types */
enum TXFrameType { TX_SPEECH_GOOD = 0,
	TX_SID_FIRST,
	TX_SID_UPDATE,
	TX_NO_DATA,
	TX_SPEECH_DEGRADED,
	TX_SPEECH_BAD,
	TX_SID_BAD,
	TX_ONSET,
	TX_N_FRAMETYPES		/* number of frame types */
};

/* Declaration of interface structure */
typedef struct {
	int16_t sid_update_counter;	/* Number of frames since last SID */
	int16_t sid_handover_debt;	/* Number of extra SID_UPD frames to schedule */
	int32_t dtx;
	enum TXFrameType prev_ft;	/* Type of the previous frame */
	void *encoderState;	/* Points encoder state structure */
} enc_interface_State;

/*
 * Sid_Sync_reset
 *
 *
 * Parameters:
 *    st                O: state structure
 *
 * Function:
 *    Initializes state memory
 *
 * Returns:
 *    void
 */
static void Sid_Sync_reset(enc_interface_State * st)
{
	st->sid_update_counter = 3;
	st->sid_handover_debt = 0;
	st->prev_ft = TX_SPEECH_GOOD;
}

/*
 * Encoder_Interface_init
 *
 *
 * Parameters:
 *    dtx               I: DTX flag
 *
 * Function:
 *    Allocates state memory and initializes state memory
 *
 * Returns:
 *    pointer to encoder interface structure
 */
void *Encoder_Interface_init(int dtx)
{
	enc_interface_State *s;

	/* allocate memory */
	if ((s = calloc(1, sizeof(enc_interface_State))) ==
	    NULL) {
		fprintf(stderr, "Encoder_Interface_init: "
			"can not malloc state structure\n");
		return NULL;
	}
	s->encoderState = Speech_Encode_Frame_init(dtx);
	Sid_Sync_reset(s);
	s->dtx = dtx;
	return s;
}

/*
 * DecoderInterfaceExit
 *
 *
 * Parameters:
 *    state             I: state structure
 *
 * Function:
 *    The memory used for state memory is freed
 *
 * Returns:
 *    Void
 */
void Encoder_Interface_exit(void *state)
{
	enc_interface_State *s;
	s = (enc_interface_State *) state;

	/* free memory */
	Speech_Encode_Frame_exit(&s->encoderState);
	free(s);
	s = NULL;
}

//encode using mode, pack parameter and SID-flag=0 to bytes
//return data length in bytes (5 for SID data for all modes)
//set force_speech=1 for cbr (no VAD function)
int AMR_encode(void *st, uint8_t mode, int16_t * speech, uint8_t * serial,
	       int force_speech)
{

	int16_t prm[PRMNO_MR122];	//parameters outputed by encoder
	enc_interface_State *s;	//pointer to state
	int i, j, noHoming = 0;	//homing detect flag
	int16_t bits;		//output bits counter

	/*  Modes list
	   Mode 0: 95 (12)
	   Mode 1: 103 (13)
	   Mode 2: 118 (15)
	   Mode 3: 134 (17)
	   Mode 4: 148 (19)
	   Mode 5: 159 (20)
	   Mode 6: 204 (26)
	   Mode 7: 244 (31)
	 */
//MRDTX 35 bits 5 bytes 5 bits free
//MR475 95 bits 12 bytes 1 bit free
//MR515 103 bits 13 bytes 1 bit free
//MR59  118 bits 15 bytes 2 bits free
//MR67  134 bits 17 bytes 2 bits free
//MR74  148 bits  19 bytes 4 bits free
//MR795 159 bits  20 bytes 1 bit free
//MR102 204 bits 26 bytes 4 bits free
//MR122 244 bits 31 bytes 4 bits free
	//Homing frames length in bytes for each mode
	uint8_t homing_size_m[8] = { 7, 7, 7, 7, 7, 8, 12, 18 };

	//Pointers to homing frames patterns for each mode
	const int16_t *dhf_m[8] = { dhf_MR475, dhf_MR515, dhf_MR59,
		dhf_MR67, dhf_MR74, dhf_MR795, dhf_MR102, dhf_MR122
	};

	//Number of parameters for each mode
	uint8_t prmno_m[8] = { PRMNO_MR475, PRMNO_MR515, PRMNO_MR59,
		PRMNO_MR67, PRMNO_MR74, PRMNO_MR795, PRMNO_MR102, PRMNO_MR122
	};

	//Number of bits in parameters for each mode
	const int16_t *bitno_m[8] = { bitno_MR475, bitno_MR515, bitno_MR59,
		bitno_MR67, bitno_MR74, bitno_MR795, bitno_MR102, bitno_MR122
	};

	//Encodec blocks length in byte for each mode  
	uint8_t bl_size_m[8] = { 12, 13, 15, 17, 19, 20, 26, 31 };

//Set variables for current mode   
	uint8_t homing_size = homing_size_m[mode];
	const int16_t *dhf = dhf_m[mode];
	uint8_t prmno = prmno_m[mode];
	const int16_t *bitno = bitno_m[mode];
	uint8_t bl_size = bl_size_m[mode];

	/*
	 * used encoder mode,
	 * if used_mode == -1, force VAD on
	 */
	enum Mode used_mode = -force_speech;

	//encoder's state 
	s = (enc_interface_State *) st;

	/*
	 * Checks if all samples of the input frame matches the encoder
	 * homing frame pattern, which is 0x0008 for all samples.
	 */
	for (i = 0; i < 160; i++) {
		noHoming = speech[i] ^ 0x0008;

		if (noHoming)
			break;
	}

//process speech frame: used_mode returns type: MRDTX for SID or current mode for speech 
	if (noHoming)
		Speech_Encode_Frame(s->encoderState, (enum Mode)mode, speech,
				    prm, &used_mode);
	else			//process homing
	{
		for (i = 0; i < homing_size; i++)
			prm[i] = dhf[i];	//set homing for decoder
		memzero(&prm[homing_size], (prmno - homing_size) << 1);	//rest of the parameters are zero
		used_mode = (enum Mode)mode;	//mark as normal frame
		Speech_Encode_Frame_reset(s->encoderState, s->dtx);	//reset encoder
		Sid_Sync_reset(s);	//reset DTX
	}

//pack parameters to bits
	memzero(serial, bl_size);	//clear memory for bits block

	if (used_mode == MRDTX)	//pack SID to 5 bits 
	{
		bits = 5;	//reserve 5 bits for SID flag=1 (bit 0) and DTX counter
		for (i = 0; i < PRMNO_MRDTX; i++)	//process each parameter
		{
			for (j = 0; j < bitno_MRDTX[i]; j++)	//process eack bits in it
			{	//set corresponding bit in byte
				if (prm[i] & ((int16_t) 1 << j))
					serial[bits >> 3] |=
					    ((uint8_t) 1 << (bits & 0x07));
				bits++;	//updates bits counter
			}
		}
		serial[0] |= 0x01;	//set dtx flag   
		return 5;	//fixed length of SID block for all modes


	} else			//this was a speech frame
	{
		bits = 1;	//reserve first bit for no-SID flag=0 (bit 0)
		for (i = 0; i < prmno; i++)	//pack speech to bits
		{
			for (j = 0; j < bitno[i]; j++) {
				if (prm[i] & ((int16_t) 1 << j))
					serial[bits >> 3] |=
					    ((uint8_t) 1 << (bits & 0x07));
				bits++;	//updates bits counter
			}
		}
		return bl_size;	//size of encoded block for current mode
	}
}

