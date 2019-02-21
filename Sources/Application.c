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
#include "AccelSensor.h"
#include "Shell.h"



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

static void APP_main_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  LightChannels_t channels;
  AccelAxis_t accelAxis;

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  LED1_Neg();
	  //LightSensor_getChannelValuesBlocking(&channels,LightSensor_Bank0_X_Y_B_B);
	  AccelSensor_getValues(&accelAxis);
	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(1000));
  } /* for */
}


void APP_Run(void) {

	LightSensor_init();
	AccelSensor_init();
	SHELL_Init();

	if (xTaskCreate(APP_main_task, "MainTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	vTaskStartScheduler();
	for(;;) {}
}
