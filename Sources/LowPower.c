/*
 * LowPower.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 *
 * NXP Application notes to LowPower: AN4470 & AN4503
 *
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

		  //OSC_CR &= ~OSC_CR_EREFSTEN_MASK; // Disable Ext. Clock in StopMode

		  SMC_PMPROT = SMC_PMPROT_ALLS_MASK;	//Allow LLS (LowLeakageStop) Datasheet p355
		  SMC_PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;
		  SMC_PMCTRL &= ~SMC_PMCTRL_RUNM(0x3);
		  SMC_PMCTRL |=  SMC_PMCTRL_STOPM(0x3); //Set the STOPM field to 0b011 for LLS mode

		  //Choose LLS2 or LLS3 :
		  SMC_STOPCTRL &=  ~SMC_STOPCTRL_LLSM_MASK;
		  SMC_STOPCTRL |=  SMC_STOPCTRL_LLSM(3); // LLS3=3, LLS2=2

		  volatile unsigned int dummyread;		// wait for write to complete to SMC before stopping core
		  dummyread = SMC_PMCTRL;				// AN4503 p.26
		  SCB_SCR |= SCB_SCR_SLEEPDEEP_MASK;	// Set the SLEEPDEEP bit to enable deep sleep mode
		  __asm volatile("wfi");				// start entry into low-power mode
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
	stopModeAllowed = TRUE;
	CS1_ExitCritical();
}

void LowPower_DisableStopMode(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	stopModeAllowed = FALSE;
	CS1_ExitCritical();
}

bool LowPower_StopModeIsEnabled(void)
{
	bool temp;
	CS1_CriticalVariable();
	CS1_EnterCritical();
	temp = stopModeAllowed;
	CS1_ExitCritical();
	return temp;
}

void LLWU_ISR(void)
{
	 //NXP Application notes to LowPower: AN4470 & AN4503
	 //Clear interrupt Flag: Wakeup Source was LowPowerTimer
	 if (LLWU_F3 & LLWU_F3_MWUF0_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F3 |= LLWU_F3_MWUF0_MASK; //Clear WakeUpInt Flag
	 }

	 //Clear interrupt Flag: Wakeup Source was RTC Alarm Interrupt
	 if (LLWU_F3 & LLWU_F3_MWUF5_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F3 |= LLWU_F3_MWUF5_MASK; //Clear WakeUpInt Flag
		 RTC_ALARM_ISR();
	 }

	 //Clear interrupt Flag: Wakeup Source was UserButton
	 else if (LLWU_F2 & LLWU_F2_WUF11_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F2 |= LLWU_F2_WUF11_MASK; //Clear WakeUpInt Flag
		 //Trigger ExtInt behavour, because the actual Interrupt is Disables in StopMode:
		 ExtInt_UI_BTN_OnInterrupt();
	 }

	 //Clear interrupt Flag: Wakeup Source was LightSensor Interrupt
	 else if (LLWU_F2 & LLWU_F2_WUF12_MASK)//Reset Interrupt Flag Datasheet p393
	 {
		 LLWU_F2 |= LLWU_F2_WUF12_MASK; //Clear WakeUpInt Flag
		 //Trigger ExtInt behavior, because the actual Interrupt is Disables in StopMode:
		 ExtInt_LI_DONE_OnInterrupt();
	 }

	volatile unsigned int dummyread; //To make Sure, the Wakeup flag is cleared
	dummyread = SMC_PMCTRL;

	  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
	     exception return operation might vector to incorrect interrupt */
	__DSB();
}
