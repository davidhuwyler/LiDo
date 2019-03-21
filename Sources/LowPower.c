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
#include "Application.h"
#include "Events.h"
#include "UI.h"

static bool stopModeAllowed = FALSE;

void LowPower_EnterLowpowerMode(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	if(stopModeAllowed)
	{
		  CS1_ExitCritical();
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
		CS1_ExitCritical();
	    __asm volatile("dsb");
	    __asm volatile("wfi");
	    __asm volatile("isb");
	}
}

void LowPower_EnableStopMode(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
#ifdef CONFIG_ENABLE_STOPMODE
	stopModeAllowed = TRUE;
#else
	stopModeAllowed = FALSE;
#endif
	CS1_ExitCritical();
}

void LowPower_DisableStopMode(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	stopModeAllowed = FALSE;
	CS1_ExitCritical();
}


void LLWU_ISR(void)
{
	//LLWU_F3 |= LLWU_F3_MWUF0_MASK; //Reset Interrupt Flag Datasheet p393
	//LLWU_PDD_WriteFlag3Reg(LLWU_DEVICE,LLWU_F3_MWUF0_MASK);
	//LLWU_F3

	 //Clear interrupt Flag: Wakeup Source was LowPowerTimer
	 if (LLWU_F3 & LLWU_F3_MWUF0_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F3 |= LLWU_F3_MWUF0_MASK; //Clear WakeUpInt Flag
	 }

	 //Clear interrupt Flag: Wakeup Source was UserButton
	 else if (LLWU_F2 & LLWU_F2_WUF11_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F2 |= LLWU_F2_WUF11_MASK; //Clear WakeUpInt Flag
		 //Trigger ExtInt behavour, bacause the actual Interrupt is Disables in StopMode:
		 ExtInt_UI_BTN_OnInterrupt();
	 }

	 //Clear interrupt Flag: Wakeup Source was LightSensor Interrupt
	 else if (LLWU_F2 & LLWU_F2_WUF12_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 //UI_ButtonCounter();
		 LLWU_F2 |= LLWU_F2_WUF12_MASK; //Clear WakeUpInt Flag
		 //Trigger ExtInt behavour, bacause the actual Interrupt is Disables in StopMode:
		 ExtInt_LI_DONE_OnInterrupt();
	 }

	volatile unsigned int dummyread;
	dummyread = SMC_PMCTRL;

	  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
	     exception return operation might vector to incorrect interrupt */
	__DSB();
}
