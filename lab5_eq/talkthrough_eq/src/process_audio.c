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
#include "eq_rt.h"      /* Lab 5, tema 6: stereo ekvilajzer */

/* Izbor ulaza za EQ: 0 = zbir ADC1 + ADC2 (radi bez obzira na koji je ulaz
 * prikljucen izvor), 1 = samo ADC1, 2 = samo ADC2. */
#define EQ_INPUT  0

void  process_AD1939_samples(  uint32_t iid, void *handlerarg)
{
	float L, R;

	eq_rt_isr_begin();		/* mjerenje ISR-a + brojac propustenih rokova */

	Receive_ADC_Samples();

#if EQ_INPUT == 1
	L = Left_Channel_In1;
	R = Right_Channel_In1;
#elif EQ_INPUT == 2
	L = Left_Channel_In2;
	R = Right_Channel_In2;
#else
	L = Left_Channel_In1 + Left_Channel_In2;
	R = Right_Channel_In1 + Right_Channel_In2;
#endif

	eq_rt_process(&L, &R);	/* stereo EQ (verzija i K se biraju iz debagera) */

	/* isti obradjeni signal na sva 4 stereo DAC izlaza */
	Left_Channel_Out1 = L;	Right_Channel_Out1 = R;
	Left_Channel_Out2 = L;	Right_Channel_Out2 = R;
	Left_Channel_Out3 = L;	Right_Channel_Out3 = R;
	Left_Channel_Out4 = L;	Right_Channel_Out4 = R;

	Transmit_DAC_Samples();

	eq_rt_isr_end();
}
