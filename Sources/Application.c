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
#include "AppDataFile.h"
#include "ExtInt_UI_BTN.h"
#include "CRC8.h"
#include "RTC1.h"

static liDoSample_t sample;
static SemaphoreHandle_t sampleSemaphore;

static void APP_main_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  LightChannels_t light;
  AccelAxis_t accelAndTemp;
  int32_t unixTimestamp;
  uint8_t samplingIntervall;

  sampleSemaphore = xSemaphoreCreateBinary();

  LightSensor_init();
  AccelSensor_init();
  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  if(AppDataFile_GetSamplingEnabled())
	  {
		  xSemaphoreTake(sampleSemaphore,pdMS_TO_TICKS(20));
		  LED1_Neg();
		  RTC_getTimeUnixFormat(&unixTimestamp);
		  sample.unixTimeStamp = unixTimestamp;
		  LightSensor_getChannelValuesBlocking(&light,LightSensor_Bank1_X_Y_Z_IR);
		  sample.lightChannelX = light.xChannelValue;
		  sample.lightChannelY = light.yChannelValue;
		  sample.lightChannelZ = light.zChannelValue;
		  sample.lightChannelIR = light.nChannelValue;
		  LightSensor_getChannelValuesBlocking(&light,LightSensor_Bank0_X_Y_B_B);
		  sample.lightChannelB440 = light.nChannelValue;
		  sample.lightChannelB490 = light.zChannelValue;
		  AccelSensor_getValues(&accelAndTemp);
		  sample.accelX = accelAndTemp.xValue;
		  sample.accelY = accelAndTemp.yValue;
		  sample.accelZ = accelAndTemp.zValue;
		  if(ExtInt_UI_BTN_GetVal() == false)
		  {
			  sample.temp = accelAndTemp.temp | 0x80;
		  }
		  else
		  {
			  sample.temp = accelAndTemp.temp;
		  }
		  crc8_liDoSample(&sample);
		  xSemaphoreGive(sampleSemaphore);
	  }

	  if(DebugWaitOnStartPin_GetVal())
	  {
		  SHELL_EnableShellFor20s();
	  }

	  AppDataFile_GetSampleIntervall(&samplingIntervall);
	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(samplingIntervall*1000));
  } /* for */
}

uint8_t APP_getCurrentSample(liDoSample_t* curSample)
{
	if ( xSemaphoreTake(sampleSemaphore, pdMS_TO_TICKS(500)) == pdTRUE)
	{
		*curSample = sample;
		xSemaphoreGive(sampleSemaphore);
	}
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
