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

void RTC_DisableRTCInterrupt(void) {
  /*! \todo */
}

void RTC_ALARM_ISR(void) {
  //LED_R_Neg(); /* debugging only */
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

    AppDataFile_GetSampleIntervall(&sampleIntervall);
    RTC_TAR = RTC_TSR + sampleIntervall - 1;    // Set Next RTC Alarm
    APP_resumeSampleTaskFromISR();
  }
  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
     exception return operation might vector to incorrect interrupt */
  __DSB();
}

void RTC_init(bool softInit) {
	(void)RTC1_Init(NULL, softInit);
}
