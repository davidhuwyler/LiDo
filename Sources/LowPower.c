/*
 * LowPower.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */
#include "LowPower.h"
#include "Cpu.h"
#include "LLWU.h"
#include "LLWU_PDD.h"
#include "LED1.h"
#include "WAIT1.h"
#include "CLS1.h"

static bool stopModeAllowed = false;

void LowPower_EnterLowpowerMode(void)
{
	if(stopModeAllowed)
	{
		LED1_Off();

		  SMC_PMPROT = SMC_PMPROT_ALLS_MASK;	//Allow LLS (LowLeakageStop) Datasheet p355
		  SMC_PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;
		  SMC_PMCTRL &= ~SMC_PMCTRL_RUNM(0x3);
		  SMC_PMCTRL |=  SMC_PMCTRL_STOPM(0x3); //Set the STOPM field to 0b011 for LLS mode

		  //Choose LLS2 or LLS3 :
		  SMC_STOPCTRL &=  ~SMC_STOPCTRL_LLSM_MASK;
		  SMC_STOPCTRL |=  SMC_STOPCTRL_LLSM(3); // LLS3=3, LLS2=2

		  volatile unsigned int dummyread;
		  dummyread = SMC_PMCTRL;
		  SCB_SCR |= SCB_SCR_SLEEPDEEP_MASK;
		  __asm volatile("wfi");
	}
	else
	{
		LED1_On();

	    __asm volatile("dsb");
	    __asm volatile("wfi");
	    __asm volatile("isb");
	}

	//Clear WakeUp Flag
	 if (LLWU_F3 & LLWU_F3_MWUF0_MASK)
	 {
		 LLWU_F3 |= LLWU_F3_MWUF0_MASK;
	 }

}

void LowPower_EnableStopMode(void)
{
	//stopModeAllowed = true;
}

void LowPower_DisableStopMode(void)
{
	stopModeAllowed = false;
}


void LLWU_ISR(void)
{
	//LLWU_F3 |= LLWU_F3_MWUF0_MASK; //Reset Interrupt Flag Datasheet p393
	//LLWU_PDD_WriteFlag3Reg(LLWU_DEVICE,LLWU_F3_MWUF0_MASK);
	//LLWU_F3

	 if (LLWU_F3 & LLWU_F3_MWUF0_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F3 |= LLWU_F3_MWUF0_MASK;
	 }


	volatile unsigned int dummyread;
	dummyread = SMC_PMCTRL;
}
