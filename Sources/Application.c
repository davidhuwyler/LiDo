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
#include "UI.h"
#include "DebugWaitOnStartPin.h"
#include "SDEP.h"
#include "AppDataFile.h"
#include "ExtInt_UI_BTN.h"
#include "CRC8.h"
#include "RTC1.h"
#include "TmDt1.h"

static SemaphoreHandle_t sampleSemaphore;
static bool setOneMarkerInLog = false;
static bool toggleEnablingSampling = false;
static bool requestForSoftwareReset = false;

void APP_setMarkerInLog(void)
{
	setOneMarkerInLog = true;
}

void APP_toggleEnableSampling(void)
{
	toggleEnablingSampling = true;
}

void APP_requestForSoftwareReset(void)
{
	requestForSoftwareReset = true;
}

static void APP_toggleEnableSamplingIfRequested(void)
{
	if(toggleEnablingSampling)
	{
		toggleEnablingSampling = false;
		if(AppDataFile_GetSamplingEnabled())
		{
			AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0");
		}
		else
		{
			AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"1");
		}
	}
}

static void APP_softwareResetIfRequested(lfs_file_t* file)
{
	if(requestForSoftwareReset)
	{
		requestForSoftwareReset = false;
		FS_closeLiDoSampleFile(file);
		//TODO deinit Stuff...
		KIN1_SoftwareReset();
	}
}

uint8_t APP_getCurrentSample(liDoSample_t* sample)
{
	  LightChannels_t light;
	  AccelAxis_t accelAndTemp;
	  int32_t unixTimestamp;
	  if(xSemaphoreTake(sampleSemaphore,pdMS_TO_TICKS(500)))
	  {
		  LED1_Neg();
		  RTC_getTimeUnixFormat(&unixTimestamp);
		  sample->unixTimeStamp = unixTimestamp;
		  LightSensor_getChannelValuesBlocking(&light,LightSensor_Bank1_X_Y_Z_IR);
		  sample->lightChannelX = light.xChannelValue;
		  sample->lightChannelY = light.yChannelValue;
		  sample->lightChannelZ = light.zChannelValue;
		  sample->lightChannelIR = light.nChannelValue;
		  LightSensor_getChannelValuesBlocking(&light,LightSensor_Bank0_X_Y_B_B);
		  sample->lightChannelB440 = light.nChannelValue;
		  sample->lightChannelB490 = light.zChannelValue;
		  AccelSensor_getValues(&accelAndTemp);
		  sample->accelX = accelAndTemp.xValue;
		  sample->accelY = accelAndTemp.yValue;
		  sample->accelZ = accelAndTemp.zValue;
		  if(setOneMarkerInLog)
		  {
			  setOneMarkerInLog =false;
			  sample->temp = accelAndTemp.temp | 0x80;
		  }
		  else
		  {
			  sample->temp = accelAndTemp.temp;
		  }
		  crc8_liDoSample(sample);
		  xSemaphoreGive(sampleSemaphore);
		  return ERR_OK;
	  }
	  else
	  {
		  return ERR_FAILED;
	  }
}

static bool APP_newDay(void)
{
	 static DATEREC oldDate;
	 DATEREC newDate;

	 TmDt1_GetDate(&newDate);
	 if(newDate.Day != oldDate.Day)
	 {
		 oldDate = newDate;
		 return true;
	 }
	 return false;

}


static void APP_main_task(void *param) {
  static bool fileIsOpen = false;
  (void)param;
  TickType_t xLastWakeTime;
  liDoSample_t sample;
  uint8_t samplingIntervall;
  lfs_file_t sampleFile;

  sampleSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(sampleSemaphore);
  LightSensor_init();
  AccelSensor_init();

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();

	  APP_softwareResetIfRequested(&sampleFile);
	  APP_toggleEnableSamplingIfRequested();
	  //New Day: Make new File!
	  if(APP_newDay() && fileIsOpen && AppDataFile_GetSamplingEnabled())
	  {
		  FS_closeLiDoSampleFile(&sampleFile);
		  FS_openLiDoSampleFile(&sampleFile);
	  }

	  //OpenFile if needed
	  if(AppDataFile_GetSamplingEnabled() && !fileIsOpen)
	  {
		  if(FS_openLiDoSampleFile(&sampleFile) == ERR_OK)
		  {
			  fileIsOpen = true;
		  }
	  }

	  if(AppDataFile_GetSamplingEnabled() && fileIsOpen)
	  {
		  APP_getCurrentSample(&sample);
		  FS_writeLiDoSample(&sample,&sampleFile);
	  }
	  else if (fileIsOpen)
	  {
		  if(FS_closeLiDoSampleFile(&sampleFile) == ERR_OK)
		  {
			  fileIsOpen = false;
		  }
	  }

	  if(DebugWaitOnStartPin_GetVal())
	  {
		  SHELL_EnableShellFor20s();
	  }

	  AppDataFile_GetSampleIntervall(&samplingIntervall);
	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(samplingIntervall*1000));
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
	UI_Init();

	if (xTaskCreate(APP_main_task, "MainTask", 5000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	vTaskStartScheduler();
	for(;;) {}
}
