/*
 * Application.c
 *
 *  Created on: 17.02.2018
 *      Author: Erich Styger
 */

#include "Application.h"
#include "FRTOS1.h"
#include "LED1.h"
#include "WAIT1.h"
#include "KIN1.h"
#include "LightSensor.h"

static uint32_t prevCycleCounter, cycleCntCounter = 0;

void AppConfigureTimerForRuntimeStats(void) {
  cycleCntCounter = 0;
  KIN1_InitCycleCounter();
  prevCycleCounter = KIN1_GetCycleCounter();
}

uint32_t AppGetRuntimeCounterValueFromISR(void) {
  uint32_t newCntr, diff;

  newCntr = KIN1_GetCycleCounter();
  diff = newCntr-prevCycleCounter;
  prevCycleCounter = newCntr;
  cycleCntCounter += diff>>12; /* scale down the counter */
  return cycleCntCounter;
}


void APP_Run(void) {
	LightSensor_init();
	vTaskStartScheduler();
	for(;;) {}
}
