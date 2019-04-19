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
 * interf_dec.c
 *
 *
 * Project:
 *     AMR Floating-Point Codec
 *
 * Contains:
 *    This module provides means to conversion from 3GPP or ETSI
 *    bitstream to AMR parameters
 */

/*
 * include files
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "amr_speech_importance.h"
#include "sp_dec.h"
#include "interf_rom.h"
#include "rom_dec.h"
#include "ophtools.h"

/*
 * definition of constants
 */
#define EHF_MASK 0x0008		/* encoder homing frame pattern */
typedef

    struct {
	int reset_flag_old;	/* previous was homing frame */

	enum RXFrameType prev_ft;	/* previous frame type */
	enum Mode prev_mode;	/* previous mode */
	void *decoder_State;	/* Points decoder state */

} dec_interface_State;

/*
 * Decoder_Interface_reset
 *
 *
 * Parameters:
 *    st                O: state struct
 *
 * Function:
 *    Reset homing frame counter
 *
 * Returns:
 *    void
 */
void Decoder_Interface_reset(dec_interface_State * st)
{
	st->reset_flag_old = 1;
	st->prev_ft = RX_SPEECH_GOOD;
	st->prev_mode = MR475;	/* minimum bitrate */
}

/*
 * Decoder_Interface_init
 *
 *
 * Parameters:
 *    void
 *
 * Function:
 *    Allocates state memory and initializes state memory
 *
 * Returns:
 *    success           : pointer to structure
 *    failure           : NULL
 */
void *Decoder_Interface_init(void)
{
	dec_interface_State *s;

	/* allocate memory */
	if ((s = calloc(1, sizeof(dec_interface_State))) ==
	    NULL) {
		fprintf(stderr, "Decoder_Interface_init: "
			"can not malloc state structure\n");
		return NULL;
	}
	s->decoder_State = Speech_Decode_Frame_init();

	if (s->decoder_State == NULL) {
		free(s);
		return NULL;
	}
	Decoder_Interface_reset(s);
	return s;
}

/*
 * Decoder_Interface_exit
 *
 *
 * Parameters:
 *    state                I: state structure
 *
 * Function:
 *    The memory used for state memory is freed
 *
 * Returns:
 *    Void
 */
void Decoder_Interface_exit(void *state)
{
	dec_interface_State *s;
	s = (dec_interface_State *) state;

	/* free memory */
	Speech_Decode_Frame_exit(s->decoder_State);
	free(s);
	s = NULL;
}

//decode using mode and bfi flag=1 (for replace lossed packets)
//or alternatively bad frame is synth=0 or four zeroes 
void AMR_decode(void *st, uint8_t mode, uint8_t * serial, int16_t * synth, int bfi)
{

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

//Set variables for current mode   
	uint8_t homing_size = homing_size_m[mode];
	const int16_t *dhf = dhf_m[mode];
	uint8_t prmno = prmno_m[mode];
	const int16_t *bitno = bitno_m[mode];

	enum Mode speech_mode = mode;	/* speech mode */
	enum RXFrameType frame_type = RX_SPEECH_GOOD;	/* frame type */
	int16_t prm[PRMNO_MR122];	/* AMR parameters */
	dec_interface_State *s;	/* pointer to structure */
	int32_t i, j;		/* counters */
	int32_t resetFlag = 1;	/* homing frame */
	uint8_t temp[31] = { 0 };	//used if serial ommited
	int16_t bits;		//bits counter

	s = (dec_interface_State *) st;	//state

	if (!serial)
		serial = temp;	//process bad frame if parameter ommited
	if (0x01 & serial[0])	//check SID flag
	{
		speech_mode = MRDTX;
		frame_type = RX_SID_FIRST;	//mark as short sid frame
	}
	if (!(*((unsigned int *)serial)))
		bfi = 1;	//set bfi if zeroed data (or ommited)
	if (bfi == 1)
		frame_type = RX_SPEECH_BAD;	//set type for CN generation

	//unpack bits ro parameters
	memzero(prm, PRMNO_MR122 * sizeof(int16_t));
	if (speech_mode == MRDTX)	//for sid frame (5 bytes)
	{
		bits = 5;	//reserve 5 bits for SID flag and counter)
		for (i = 0; i < PRMNO_MRDTX; i++)	//read each parameter
		{
			for (j = 0; j < bitno_MRDTX[i]; j++)	//read each bit 
			{
				if (serial[bits >> 3] &
				    (((uint8_t) 1) << (bits & 7)))
					prm[i] |= ((int16_t) 1 << j);
				bits++;
			}
		}
	} else			//for speech frame
	{
		bits = 1;	//skip first bit (SID flag)
		for (i = 0; i < prmno; i++) {
			for (j = 0; j < bitno[i]; j++) {
				if (serial[bits >> 3] &
				    (((uint8_t) 1) << (bits & 7)))
					prm[i] |= ((int16_t) 1 << j);
				bits++;
			}
		}
	}
/* test for homing frame */
	if (s->reset_flag_old == 1)
		for (i = 0; i < homing_size; i++) {
			resetFlag = prm[i] ^ dhf[i];
			if (resetFlag)
				break;
		}
	if ((resetFlag == 0) && (s->reset_flag_old != 0))	//duplicate homing - skip it
		for (i = 0; i < 160; i++)
			synth[i] = EHF_MASK;
	else
		Speech_Decode_Frame(s->decoder_State, speech_mode, prm, frame_type, synth);	//decode
	/* check whole frame for homing */
	if (s->reset_flag_old == 0)
		for (i = 0; i < prmno; i++) {
			resetFlag = prm[i] ^ dhf[i];
			if (resetFlag)
				break;
		}
	if (resetFlag == 0)
		Speech_Decode_Frame_reset(s->decoder_State);	//reset decoder  
	s->reset_flag_old = !resetFlag;	//store in state
	s->prev_ft = frame_type;
	s->prev_mode = speech_mode;
}

