/*********************************************************************************

 Copyright(c) 2012 Analog Devices, Inc. All Rights Reserved.

 This software is proprietary and confidential.  By using this software you agree
 to the terms of the associated Analog Devices License Agreement.

 *********************************************************************************/
/*This project is a talk-through example using the onboard AD1939 to acquire
 * and output an audio stream. The digital audio data is available for processing
 * in the file SPORT1_isr.c. The block size is 256 samples per audio channel.
 * The output on OUT2 is delayed and the user can expect to hear an echo effect.
 */

#include "ADDS_21479_EzKit.h"
#include "eq_rt.h"      /* Lab 5, tema 6: stereo ekvilajzer */


volatile float * DelayLine;
volatile int Index  = 0;

void	main()
{
	volatile int count=0;

	/* Initialize managed drivers and/or services at the start of main(). */
	adi_initComponents();

	/* Initialize SHARC PLL*/
	initPLL();

	/* Initialize DDR2 SDRAM controller to access memory */
	initExternalMemory();

	/* Initialize DAI because the SPORT and SPI signals need to be routed */
    InitDAI();

    /* This function will configure the AD1939 codec on the 21469 EZ-KIT */
    init1939viaSPI();

    /* Turn on SPORT0 TX and SPORT1 RX for Multichannel Operation */
	enable_SPORT01_MCM_mode();


	/* Install and enable a handler for the SPORT1 Receiver interrupt.*/
	adi_int_InstallHandler(ADI_CID_P3I,process_AD1939_samples,0,true);


	/* Enable multichannel operation (SPORT mode and DMA in standby and ready) */
	*pSPMCTL0 |= MCEA;
	*pSPMCTL1 |= MCEA;

	SinTableInit();

	/* Lab 5, tema 6: delay linija (eho) se vise ne koristi - umjesto nje radi EQ.
	 * fCCLK = 400 MHz (init_PLL_SDRAM.c), fs = 48 kHz (init1939viaSPI.c). */
	eq_rt_init(400000000.0f, 48000.0f);

	for (;;)
	  {
		if(count==10)
			puts("Talkthrough + EQ is running successfully");
		count++;
		eq_rt_poll();		/* primjena promjene g_req_version / g_req_K */
    	asm("idle;");		/* cekanje na sljedeci prekid */
      }
}

