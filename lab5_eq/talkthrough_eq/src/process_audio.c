/*********************************************************************************

 Copyright(c) 2012 Analog Devices, Inc. All Rights Reserved.

 This software is proprietary and confidential.  By using this software you agree
 to the terms of the associated Analog Devices License Agreement.

 *********************************************************************************/

/*
 * NAME:     process_audio.c (sample-based Talkthrough)
 * PURPOSE:  Function process_audio() for AD1939/ADSP-21364 Talkthrough framework.
 * USAGE:    This file contains the function to process AD1939 samples.
 */

/*
 *--------------------------------------------------------------------------------------
 *
 * DMA Buffer relationship to C Audio Variables
 *
 * Timeslot #  SPORT DMA Buffer Timeslot Data   <->   AD1939 ADC/DAC C Variables (floats)
 * ======================================================================================
 *
 * 0           DM(_rx1a_bufa + Internal_ADC_L1) -->  Left_Channel_In1
 * 1           DM(_rx1a_bufa + Internal_ADC_R1) -->  Right_Channel_In1
 * 2           DM(_rx1a_bufa + Internal_ADC_L2) -->  Left_Channel_In2
 * 3           DM(_rx1a_bufa + Internal_ADC_R2) -->  Right_Channel_In2
 * 4           DM(_rx1a_bufa + AUX_ADC_L1)      -->  X (not used in EZ-KIT Lite)
 * 5           DM(_rx1a_bufa + AUX_ADC_R1)      -->  X (not used in EZ-KIT Lite)
 * 6           DM(_rx1a_bufa + AUX_ADC_L2)      -->  X (not used in EZ-KIT Lite)
 * 7           DM(_rx1a_bufa + AUX_ADC_R2)      -->  X (not used in EZ-KIT Lite)
 *
 * 0           DM(_tx0a_buf + Internal_DAC_L1)  <--  Left_Channel_Out1
 * 1           DM(_tx0a_buf + Internal_DAC_R1)  <--  Right_Channel_Out1
 * 2           DM(_tx0a_buf + Internal_DAC_L2)  <--  Left_Channel_Out2
 * 3           DM(_tx0a_buf + Internal_DAC_R2)  <--  Right_Channel_Out2
 * 4           DM(_tx0a_buf + Internal_DAC_L3)  <--  Left_Channel_Out3
 * 5           DM(_tx0a_buf + Internal_DAC_R3)  <--  Right_Channel_Out3
 * 6           DM(_tx0a_buf + Internal_DAC_L4)  <--  Left_Channel_Out4
 * 7           DM(_tx0a_buf + Internal_DAC_R4)  <--  Right_Channel_Out4
 */



#include "ADDS_21479_EzKit.h"

void  process_AD1939_samples(  uint32_t iid, void *handlerarg)
{
	/* Perform AD1939 Audio Processing Here Using 32-bit Floating-point Math
	 *  (Note: The ADC/DAC variables below are used to "double-buffer" the audio
	 *  samples with temporary variables, allowing active DMA completion of
	 *  rx1a_buf[] and tx0a_buf[])
	 */
	   
	Receive_ADC_Samples();

	/* Loop back AD1939 ADC1 data to DAC1 */
	Left_Channel_Out1 = Left_Channel_In2;	
	Right_Channel_Out1 = Right_Channel_In2;	

	/* create a simple stereo digital delay on internal AD1939 stereo DAC2 channels */
	/* Delay-line length must not exceed usable seg_dmda memory addresses! */
	Left_Channel_Out2 = Left_Channel_In2;			/* loopback left channel, no processing */
	Right_Channel_Out2 = DelayLine[Index] + Right_Channel_In2; /* delayed left + right channel */
	
	DelayLine[Index++] = Right_Channel_In2;		/* store right channel into delay-line */
	if (Index == DELAYLINESIZE) Index = 0;

	/* loop back ADC1 audio data to other DACs 3 & 4 */
	Left_Channel_Out3 = Left_Channel_In1;
	Right_Channel_Out3 = Right_Channel_In1;
	Left_Channel_Out4 = Left_Channel_In1;
	Right_Channel_Out4 = Right_Channel_In1;

	Transmit_DAC_Samples();
}

