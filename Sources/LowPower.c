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
#include "WAIT1.h"
#include "CLS1.h"
#include "Application.h"
#include "Events.h"
#include "UI.h"
#include "LPTMR_PDD.h"

static bool stopModeAllowed = FALSE;
static unsigned char ucMCG_C1;

#define CLOCK_DIV 2
#define CLOCK_MUL 24
#define MCG_C6_VDIV0_LOWEST 24

void LowPower_EnterLowpowerMode(void)
{
#if 1
    __asm volatile("dsb");
    __asm volatile("wfi");
    __asm volatile("isb");

#else
	CS1_CriticalVariable();
	CS1_EnterCritical();

	if(stopModeAllowed)
	{
		  CS1_ExitCritical();
		  ucMCG_C1 = MCG_C1;
		  Cpu_SetOperationMode(DOM_STOP, NULL, NULL);
	}
	else
	{
		CS1_ExitCritical();
	    __asm volatile("dsb");
	    __asm volatile("wfi");
	    __asm volatile("isb");
	}
#endif
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
	LLWU_PE2 |= LLWU_PE2_WUPE5(0x1); //Enable PTB0 (LightSensor) as WakeUpSource
	LLWU_PE2 |= LLWU_PE2_WUPE6(0x1); //Enable PTC1 (UserButton) as WakeUpSouce
}

void LLWU_ISR(void)
{

	//Switch to PLL and wait for it to fully start up
	//https://community.nxp.com/thread/458972
    MCG_C5 = ((CLOCK_DIV - 1) | MCG_C5_PLLSTEN0_MASK); // move from state FEE to state PBE (or FBE) PLL remains enabled in normal stop modes
    MCG_C6 = ((CLOCK_MUL - MCG_C6_VDIV0_LOWEST) | MCG_C6_PLLS_MASK);
    while ((MCG_S & MCG_S_PLLST_MASK) == 0) {}   // loop until the PLLS clock source becomes valid
    while ((MCG_S & MCG_S_LOCK0_MASK) == 0) {}    // loop until PLL locks
    MCG_C1 = ucMCG_C1;                      // finally move from PBE to PEE mode - switch to PLL clock (the original settings are returned)
    while ((MCG_S & MCG_S_CLKST_MASK) != MCG_S_CLKST(11)) {} // loop until the PLL clock is selected

	uint32_t wakeUpFlags;
	wakeUpFlags = Cpu_GetLLSWakeUpFlags();

	if (wakeUpFlags&LLWU_INT_MODULE0)  /* LPTMR */
	{
	    LPTMR_PDD_ClearInterruptFlag(LPTMR0_BASE_PTR); /* Clear interrupt flag */
	}

	if (wakeUpFlags&LLWU_INT_MODULE5)  /* RTC Alarm */
	{
		RTC_ALARM_ISR();
	}

	if (wakeUpFlags&LLWU_EXT_PIN5)    /* PTB0 = LightSensor */
	{
		LLWU_F1 |= LLWU_F1_WUF5_MASK; //Clear WakeUpInt Flag
		LightSensor_Done_ISR();
	}

	if (wakeUpFlags&LLWU_EXT_PIN6)  /* PTC1 = UserButton */
	{
		LLWU_F1 |= LLWU_F1_WUF6_MASK; //Clear WakeUpInt Flag
		UI_ButtonPressed_ISR();
	}

	  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
	     exception return operation might vector to incorrect interrupt */
	__DSB();
}
