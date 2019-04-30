/*
 * LowPower.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 *
 * NXP Application notes to LowPower: AN4470 & AN4503
 *
 * For the K22DX256 controller:
 * https://mcuoneclipse.com/2014/03/16/starting-point-for-kinetis-low-power-lls-mode/
 */
#include "LowPower.h"
#include "Cpu.h"
#include "LED_R.h"
#include "LED_G.h" //Debug
#include "LED_B.h" //Debug
#include "WAIT1.h"
#include "CLS1.h"
#include "Application.h"
#include "Events.h"
#include "UI.h"
#include "LPTMR_PDD.h"

static bool stopModeAllowed = FALSE;

void LowPower_EnterLowpowerMode(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	if(stopModeAllowed)
	{
		  CS1_ExitCritical();

		  //OSC_CR &= ~OSC_CR_EREFSTEN_MASK; // Disable Ext. Clock in StopMode

//		  SMC_PMPROT = SMC_PMPROT_ALLS_MASK;	//Allow LLS (LowLeakageStop) Datasheet p355
//		  SMC_PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;
//		  SMC_PMCTRL &= ~SMC_PMCTRL_RUNM(0x3);
//		  SMC_PMCTRL |=  SMC_PMCTRL_STOPM(0x3); //Set the STOPM field to 0b011 for LLS mode

		  //Choose LLS2 or LLS3 :
		  //SMC_

		  //SMC_STOPCTRL &=  ~SMC_STOPCTRL_LLSM_MASK;
		  //SMC_STOPCTRL |=  SMC_STOPCTRL_LLSM(3); // LLS3=3, LLS2=2

//		  volatile unsigned int dummyread;		// wait for write to complete to SMC before stopping core
//		  dummyread = SMC_PMCTRL;				// AN4503 p.26
//		  SCB_SCR |= SCB_SCR_SLEEPDEEP_MASK;	// Set the SLEEPDEEP bit to enable deep sleep mode
//		  __asm volatile("wfi");				// start entry into low-power mode


		  Cpu_SetOperationMode(DOM_STOP, NULL, NULL);
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

void LowPower_init(void)
{
//	LLWU_PE2 |= (uint8_t)LLWU_PE2_WUPE5(0x1); //Enable PTB0 (LightSensor) as WakeUpSource
//	LLWU_PE2 |= (uint8_t)LLWU_PE2_WUPE6(0x1); //Enable PTC1 (UserButton) as WakeUpSouce
}

void LLWU_ISR(void)
{
	uint32_t wakeUpFlags;
	wakeUpFlags = Cpu_GetLLSWakeUpFlags();

	//LED_G_Neg();
	if (wakeUpFlags&LLWU_INT_MODULE0)  /* LPTMR */
	{   LED_G_Neg();

	    LPTMR_PDD_ClearInterruptFlag(LPTMR0_BASE_PTR); /* Clear interrupt flag */
	}

	if (wakeUpFlags&LLWU_INT_MODULE5)  /* RTC Alarm */
	{
		LED_R_Neg();

		RTC_ALARM_ISR();
	}


//
//	if (wakeUpFlags&LLWU_EXT_PIN5)    /* PTB0 = LightSensor */
//	{
//		LED_G_Neg();
//		LLWU_F1 |= LLWU_F1_WUF5_MASK; //Clear WakeUpInt Flag
//		LightSensor_Done_ISR();
//	}
//
//	if (wakeUpFlags&LLWU_EXT_PIN6)  /* PTC1 = UserButton */
//	{
//		LED_B_Neg();
//		LLWU_F1 |= LLWU_F1_WUF6_MASK; //Clear WakeUpInt Flag
//		UI_ButtonPressed_ISR();
//	}






//	 //NXP Application notes to LowPower: AN4470 & AN4503
//	 //Clear interrupt Flag: Wakeup Source was LowPowerTimer
//	 if (LLWU_F3 & LLWU_F3_MWUF0_MASK)//Reset Interrupt Flag Datasheet p393
//	 {
//		 LPTMR0_CSR |= LPTMR_CSR_TCF_MASK; //Clear Timer Flag
//		 //LLWU_F3 |= LLWU_F3_MWUF0_MASK; //Clear WakeUpInt Flag
//		 UI_LEDpulse(LED_W);
//	 }
//
//	 //Clear interrupt Flag: Wakeup Source was RTC Alarm Interrupt
//	 if (LLWU_F3 & LLWU_F3_MWUF5_MASK)//Reset Interrupt Flag Datasheet p393
//	 {
//		 //LLWU_F3 |= LLWU_F3_MWUF5_MASK; //Clear WakeUpInt Flag
//		 RTC_ALARM_ISR();
//		 UI_LEDpulse(LED_W);
//	 }
//
//	 //Clear interrupt Flag: Wakeup Source was UserButton
//	 else if (LLWU_F1 & LLWU_F1_WUF6_MASK)//Reset Interrupt Flag Datasheet p300
//	 {
//		 LLWU_F1 |= LLWU_F1_WUF6_MASK; //Clear WakeUpInt Flag
//		 //Trigger ExtInt behavour, because the actual Interrupt is Disables in StopMode:
//		 UI_ButtonPressed_ISR();
//		 UI_LEDpulse(LED_W);
//	 }
//
//	 //Clear interrupt Flag: Wakeup Source was LightSensor Interrupt
//	 else if (LLWU_F1 & LLWU_F1_WUF5_MASK)//Reset Interrupt Flag Datasheet p393
//	 {
//		 LLWU_F1 |= LLWU_F1_WUF5_MASK; //Clear WakeUpInt Flag
//		 //Trigger ExtInt behavior, because the actual Interrupt is Disabled in StopMode:
//		 LightSensor_Done_ISR();
//		 UI_LEDpulse(LED_W);
//	 }

//	volatile unsigned int dummyread; //To make Sure, the Wakeup flag is cleared
//	dummyread = SMC_PMCTRL;

	  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
	     exception return operation might vector to incorrect interrupt */
	__DSB();
}
