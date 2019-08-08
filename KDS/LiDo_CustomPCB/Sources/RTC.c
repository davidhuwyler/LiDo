/*
 * RTC.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */
#include "Platform.h"
#include "Application.h"
#include "AppDataFile.h"
#include "RTC.h"
#include "RTC1.h"
#include "TmDt1.h"
#include "LED_R.h"

static LDD_TDeviceData *rtcHandle;

void RTC_getTimeUnixFormat(int32_t *rtcTimeUnix) {
	TIMEREC time;
	DATEREC date;

	TmDt1_GetInternalRTCTimeDate(&time, &date);
	*rtcTimeUnix = TmDt1_TimeDateToUnixSeconds(&time, &date, 0);
}

void RTC_setTimeUnixFormat(int32_t rtcTimeUnix) {
	TIMEREC time;
	DATEREC date;

	TmDt1_UnixSecondsToTimeDate(rtcTimeUnix, 0, &time, &date);
	TmDt1_SetInternalRTCTimeDate(&time,&date);
}

void RTC_EnableRTCInterrupt(void) {
  /* Init the RTC alarm Interrupt: */
  RTC_CR  |= RTC_CR_SUP_MASK;   /* Write to RTC Registers enabled */
  RTC_IER |= RTC_IER_TAIE_MASK; /* Enable RTC Alarm Interrupt */
  RTC_IER |= RTC_IER_TOIE_MASK; /* Enable RTC Overflow Interrupt */
  RTC_IER |= RTC_IER_TIIE_MASK; /* Enable RTC Invalid Interrupt */
  RTC_TAR = RTC_TSR;            /* RTC Alarm at RTC Time */
}

void RTC_DisableRTCInterrupts(void) {
  /* stop RTC alarm interrupt */
  RTC_IER &= ~RTC_IER_TAIE_MASK; /* Disable RTC Alarm Interrupt */
}

void RTC_ALARM_ISR(void) {
  if (RTC_SR & RTC_SR_TIF_MASK) { /* Timer invalid (Vbat POR or RTC SW reset)? */
    RTC_SR &= ~RTC_SR_TCE_MASK;  /* Disable counter */
    RTC_TPR = 0x00U;       /* Reset prescaler */
    RTC_TSR = 0x02UL;      /* Set init. time - 2000-01-01 0:0:1 (clears flag)*/
  } else if (RTC_SR & RTC_SR_TOF_MASK) {
    RTC_SR &= ~RTC_SR_TCE_MASK;  /* Disable counter */
    RTC_TPR = 0x00U;       /* Reset prescaler */
    RTC_TSR = 0x02UL;      /* Set init. time - 2000-01-01 0:0:1 (clears flag)*/
  } else { /* Alarm interrupt */
    uint8_t sampleIntervall;

    if (AppDataFile_GetSamplingEnabled()) {
      AppDataFile_GetSampleIntervall(&sampleIntervall);
      RTC_TAR = RTC_TSR + sampleIntervall - 1;    // Set Next RTC Alarm
      APP_resumeSampleTaskFromISR();
    } else {
      RTC_DisableRTCInterrupts();
    }
  }
  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
     exception return operation might vector to incorrect interrupt */
  __DSB();
}

void RTC_Init(bool softInit) {
  rtcHandle = RTC1_Init(NULL, softInit);
	(void)RTC1_Enable(rtcHandle); /* make sure the hardware is enabled */
  RTC_EnableRTCInterrupt(); /* RTC alarm is used to wakeup sample task */
}
