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

#include "Platform.h"
#include "LowPower.h"
#include "Cpu.h"
#include "CLS1.h"
#include "Events.h"
#include "UI.h"
#include "LPTMR_PDD.h"
#include "LightSensor.h"

static volatile bool stopModeAllowed = FALSE;
static unsigned char ucMCG_C1; /* backup to restore register value */

BaseType_t xEnterTicklessIdle(void) {
#if PL_CONFIG_HAS_LOW_POWER
  return pdTRUE; /* enter tickless idle mode */
#else
  return pdFALSE; /* do not enter tickless idle mode */
#endif
}

void LowPower_EnterLowpowerMode(void) {
#if PL_CONFIG_HAS_LOW_POWER
	if(stopModeAllowed) {
    ucMCG_C1 = MCG_C1; /* remember current MCG value for the wakeup */
    Cpu_SetOperationMode(DOM_STOP, NULL, NULL);
	}	else {
    __asm volatile("dsb");
    __asm volatile("wfi");
    __asm volatile("isb");
	}
#else
  #warning "low power mode disabled!"
  __asm volatile("dsb");
  __asm volatile("wfi");
  __asm volatile("isb");
#endif
}

void LowPower_EnableStopMode(void) {
	stopModeAllowed = TRUE;
}

void LowPower_DisableStopMode(void) {
	stopModeAllowed = FALSE;
}

bool LowPower_StopModeIsEnabled(void) {
	return stopModeAllowed;
}

void LowPower_init(void) {
  /* configure LLWU (Low Leakage Wake-Up) pins */
	LLWU_PE2 |= LLWU_PE2_WUPE5(0x1); //Enable PTB0 (LightSensor) as WakeUpSource with rising edge
#if PL_BOARD_REV==20 || PL_BOARD_REV==21
	LLWU_PE2 |= LLWU_PE2_WUPE6(0x1); //Enable PTC1 (UserButton) as WakeUpSouce with rising edge
#else
  LLWU_PE4 |= LLWU_PE4_WUPE12(0x1); //Enable PTD0 (UserButton) as WakeUpSouce with rising edge
#endif
}

/* interrupt called in case of LLWU wake-up */
void LLWU_ISR(void) {
  #define CLOCK_DIV 2
  #define CLOCK_MUL 24
  #define MCG_C6_VDIV0_LOWEST 24
  uint32_t wakeUpFlags;
  wakeUpFlags = Cpu_GetLLSWakeUpFlags();
#if 0
  //Switch to PLL and wait for it to fully start up
  //https://community.nxp.com/thread/458972
  MCG_C5 = ((CLOCK_DIV - 1) | MCG_C5_PLLSTEN0_MASK); // move from state FEE to state PBE (or FBE) PLL remains enabled in normal stop modes
  MCG_C6 = ((CLOCK_MUL - MCG_C6_VDIV0_LOWEST) | MCG_C6_PLLS_MASK);
  while ((MCG_S & MCG_S_PLLST_MASK) == 0) {}   // loop until the PLLS clock source becomes valid
  while ((MCG_S & MCG_S_LOCK0_MASK) == 0) {}    // loop until PLL locks
  MCG_C1 = ucMCG_C1;                      // finally move from PBE to PEE mode - switch to PLL clock (the original settings are returned)
  while ((MCG_S & MCG_S_CLKST_MASK) != MCG_S_CLKST(11)) {} // loop until the PLL clock is selected
#endif
#if 0 /* indicate wakeup with a short blink */
	LED_R_Neg();
#elif 1
  LED_B_On();
  WAIT1_Waitus(50);
  LED_B_Off();
  //WAIT1_Waitus(50);
#endif
	if (wakeUpFlags&LLWU_INT_MODULE0) { /* LPTMR */
	  LPTMR_PDD_ClearInterruptFlag(LPTMR0_BASE_PTR); /* Clear interrupt flag */
	  LLWU_F3 |= LLWU_F3_MWUF0_MASK; /* clear WakeUpInt flag for Module0 (LPTMR) */
	  LED_R_On();
	  for(;;) {}
	  WAIT1_Waitus(50);
	  LED_R_Off();
	}
	if (wakeUpFlags&LLWU_INT_MODULE5) { /* RTC Alarm */
    LLWU_F3 |= LLWU_F3_MWUF5_MASK; /* clear WakeUpInt flag for Module5 (RTC) */
    LED_R_On();
    WAIT1_Waitus(50);
    LED_R_Off();

		RTC_ALARM_ISR();
	}
	if (wakeUpFlags&LLWU_EXT_PIN5) {   /* PTB0 = LightSensor */
		LLWU_F1 |= LLWU_F1_WUF5_MASK; //Clear WakeUpInt Flag
		LightSensor_Done_ISR();
	}
#if PL_BOARD_REV==20 || PL_BOARD_REV==21
	if (wakeUpFlags&LLWU_EXT_PIN6) { /* PTC1 = UserButton */
		LLWU_F1 |= LLWU_F1_WUF6_MASK; //Clear WakeUpInt Flag
		UI_ButtonPressed_ISR();
	}
#else
  if (wakeUpFlags&LLWU_EXT_PIN12) { /* PTD0 = UserButton */
    LLWU_F2 |= LLWU_F2_WUF12_MASK; //Clear WakeUpInt Flag
    UI_ButtonPressed_ISR();
  }
#endif
	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
	     exception return operation might vector to incorrect interrupt */
	__DSB();
}
