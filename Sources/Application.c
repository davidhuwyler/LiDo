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
#include "RTC.h"
#include "FileSystem.h"
#include "DebugWaitOnStartPin.h"
#include "SDEP.h"

static void APP_main_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  LightChannels_t channels;
  AccelAxis_t accelAxis;

  LightSensor_init();
  AccelSensor_init();


  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();

	  //LightSensor_getChannelValuesBlocking(&channels,LightSensor_Bank0_X_Y_B_B);
	  LED1_Neg();
	  AccelSensor_getValues(&accelAxis);

	  if(DebugWaitOnStartPin_GetVal())
	  {
		  SHELL_EnableShellFor20s();
	  }

	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(1000));
  } /* for */
}

void APP_Run(void) {

	//EmercencyBreak: If LowPower went wrong...
	while(DebugWaitOnStartPin_GetVal())
	{
		LED1_Neg();
		WAIT1_Waitms(50);
	}

	SHELL_Init();
	RTC_init(1);
	SDEP_Init();

	if (xTaskCreate(APP_main_task, "MainTask", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	vTaskStartScheduler();
	for(;;) {}
}
