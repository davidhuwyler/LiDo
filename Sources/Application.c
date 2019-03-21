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
#include "WatchDog.h"

static SemaphoreHandle_t sampleSemaphore;
static bool fileIsOpen = FALSE;
static bool setOneMarkerInLog = FALSE;
static volatile bool toggleEnablingSampling = FALSE;
static bool requestForSoftwareReset = FALSE;

void APP_setMarkerInLog(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	setOneMarkerInLog = TRUE;
	CS1_ExitCritical();
}

void APP_toggleEnableSampling(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	toggleEnablingSampling = TRUE;
	CS1_ExitCritical();
}

void APP_requestForSoftwareReset(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	requestForSoftwareReset = TRUE;
	CS1_ExitCritical();
}

static void APP_toggleEnableSamplingIfRequested(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	if(toggleEnablingSampling)
	{
        toggleEnablingSampling = FALSE;
    	CS1_ExitCritical();
		if(AppDataFile_GetSamplingEnabled())
		{
			AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0");
			//AppDataFile_SetSamplingEnables(false);
		}
		else
		{
            AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"1");
			//AppDataFile_SetSamplingEnables(true);
		}
	}
	else
	{
		CS1_ExitCritical();
	}
}

static void APP_softwareResetIfRequested(lfs_file_t* file)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	if(requestForSoftwareReset)
	{
		requestForSoftwareReset = FALSE;
		CS1_ExitCritical();
		if(fileIsOpen)
		{
			FS_closeLiDoSampleFile(file);
		}
		//TODO deinit Stuff...
		KIN1_SoftwareReset();
	}
	else
	{
		CS1_ExitCritical();
	}
}

uint8_t APP_getCurrentSample(liDoSample_t* sample)
{
	  LightChannels_t light;
	  AccelAxis_t accelAndTemp;
	  int32_t unixTimestamp;

	  if(xSemaphoreTake(sampleSemaphore,pdMS_TO_TICKS(500)))
	  {
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

		  CS1_CriticalVariable();
		  CS1_EnterCritical();
		  if(setOneMarkerInLog)
		  {
			  setOneMarkerInLog =FALSE;
			  CS1_ExitCritical();
			  sample->temp = accelAndTemp.temp | 0x80;
		  }
		  else
		  {
			  CS1_ExitCritical();
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
		 return TRUE;
	 }
	 return FALSE;
}


static void APP_main_task(void *param) {
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
	  WatchDog_Kick(WatchDog_KickedByApplication_c);
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
			  fileIsOpen = TRUE;
		  }
	  }

	  if(AppDataFile_GetSamplingEnabled() && fileIsOpen)
	  {
		  LED1_Neg();
		  APP_getCurrentSample(&sample);
		  FS_writeLiDoSample(&sample,&sampleFile);
	  }
	  else if (fileIsOpen)
	  {
		  if(FS_closeLiDoSampleFile(&sampleFile) == ERR_OK)
		  {
			  fileIsOpen = FALSE;
		  }
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
	WatchDog_Init();

	if (xTaskCreate(APP_main_task, "MainTask", 5000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	vTaskStartScheduler();
	for(;;) {}
}
