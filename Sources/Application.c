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
#include "SPIF.h"

static SemaphoreHandle_t sampleSemaphore;
static volatile bool fileIsOpen = FALSE;
static volatile bool setOneMarkerInLog = FALSE;
static volatile bool toggleEnablingSampling = FALSE;
static volatile bool requestForSoftwareReset = FALSE;

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
		WatchDog_StartComputationTime(WatchDog_ToggleEnableSampling);
        toggleEnablingSampling = FALSE;
    	CS1_ExitCritical();
		if(AppDataFile_GetSamplingEnabled())
		{
			AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0");
		}
		else
		{
            AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"1");
		}
		WatchDog_StopComputationTime(WatchDog_ToggleEnableSampling);
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
			FS_closeFile(file);
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
	  LightChannels_t lightB0,lightB1;
	  AccelAxis_t accelAndTemp;
	  int32_t unixTimestamp;
	  uint8_t err = ERR_OK;

	  if(xSemaphoreTake(sampleSemaphore,pdMS_TO_TICKS(500)))
	  {
		  WatchDog_StartComputationTime(WatchDog_TakeLidoSample);
		  RTC_getTimeUnixFormat(&unixTimestamp);
		  sample->unixTimeStamp = unixTimestamp;
		  if(LightSensor_getChannelValues(&lightB0,&lightB1) != ERR_OK)
		  {
			  SDEP_InitiateNewAlert(SDEP_ALERT_LIGHTSENS_ERROR);
		  }
		  sample->lightChannelX = lightB1.xChannelValue;
		  sample->lightChannelY = lightB1.yChannelValue;
		  sample->lightChannelZ = lightB1.zChannelValue;
		  sample->lightChannelIR = lightB1.nChannelValue;
		  sample->lightChannelB440 = lightB0.nChannelValue;
		  sample->lightChannelB490 = lightB0.zChannelValue;
		  if(AccelSensor_getValues(&accelAndTemp) != ERR_OK)
		  {
			  SDEP_InitiateNewAlert(SDEP_ALERT_ACCELSENS_ERROR);
		  }
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
		  WatchDog_StopComputationTime(WatchDog_TakeLidoSample);
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

static bool APP_newHour(void)
{
	 static TIMEREC oldTime;
	 TIMEREC newTime;

	 TmDt1_GetTime(&newTime);
	 if(newTime.Hour != oldTime.Hour)
	 {
		 oldTime = newTime;
		 return TRUE;
	 }
	 return FALSE;
}


static void APP_main_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  static TickType_t lastTaskExecutionDuration = 1; //Time it took to execute the task last time
  liDoSample_t sample;
  lfs_file_t sampleFile;
  uint8_t samplingIntervall;
  sampleSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(sampleSemaphore);
  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  AppDataFile_GetSampleIntervall(&samplingIntervall);
	  WatchDog_StartComputationTime(WatchDog_MeasureTaskRunns);

	  //SPIF_ReleaseFromDeepPowerDown();
	  APP_softwareResetIfRequested(&sampleFile);
	  APP_toggleEnableSamplingIfRequested();

	  //New Hour: Make new File!
	  if(APP_newHour() && fileIsOpen && AppDataFile_GetSamplingEnabled())
	  {
		  WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
		  if(FS_closeFile(&sampleFile) != ERR_OK)
		  {
			  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
		  }
		  WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
		  WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
		  if(FS_openLiDoSampleFile(&sampleFile) != ERR_OK)
		  {
			  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_openLiDoSampleFile failed");
		  }
		  WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
	  }

	  //OpenFile if needed
	  if(AppDataFile_GetSamplingEnabled() && !fileIsOpen)
	  {
		  WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
		  if(FS_openLiDoSampleFile(&sampleFile) == ERR_OK)
		  {
			  fileIsOpen = TRUE;
		  }
		  else
		  {
			  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_openLiDoSampleFile failed");
		  }
		  WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
	  }

	  if(AppDataFile_GetSamplingEnabled() && fileIsOpen)
	  {
		  LED1_Neg();
		  if(APP_getCurrentSample(&sample) != ERR_OK)
		  {
			  SDEP_InitiateNewAlert(SDEP_ALERT_SAMPLING_ERROR);
		  }

		  WatchDog_StartComputationTime(WatchDog_WriteToLidoSampleFile);
		  if(FS_writeLiDoSample(&sample,&sampleFile) != ERR_OK)
		  {
			  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_writeLiDoSample failed");
		  }
		  WatchDog_StopComputationTime(WatchDog_WriteToLidoSampleFile);
	  }
	  else if (fileIsOpen)
	  {
		  WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
		  if(FS_closeFile(&sampleFile) == ERR_OK)
		  {
			  fileIsOpen = FALSE;
		  }
		  else
		  {
			  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
		  }
		  WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
	  }

	  //SPIF_GoIntoDeepPowerDown();
	  WatchDog_StopComputationTime(WatchDog_MeasureTaskRunns);
	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(samplingIntervall*1000));
  } /* for */
}

static void APP_init_task(void *param) {
  (void)param;
	RTC_init(1);
	SDEP_Init();
	UI_Init();
	LightSensor_init();
	AccelSensor_init();
	FS_Init();
	AppDataFile_Init();
	SHELL_Init();
	WatchDog_Init();
	if (xTaskCreate(APP_main_task, "Init", 5000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}
	vTaskSuspend(xTaskGetCurrentTaskHandle());
}

void APP_Run(void) {

	//EmercencyBreak: If LowPower went wrong...
	while(DebugWaitOnStartPin_GetVal())
	{
		LED1_Neg();
		WAIT1_Waitms(50);
	}

	if (xTaskCreate(APP_init_task, "Init", 5000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	vTaskStartScheduler();
	for(;;) {}
}


static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"APP", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

static uint8_t PrintLiDoFile(uint8_t* fileNameSrc, CLS1_ConstStdIOType *io)
{
	lfs_file_t sampleFile;
	uint8_t nofReadChars;
	bool samplingWasEnabled = FALSE;
	uint8_t sampleBuf[LIDO_SAMPLE_SIZE];
	uint8_t samplePrintLine[120];
	uint16_t sampleNr = 0;
	int32 unixTimeStamp;
	TIMEREC time;
	DATEREC date;

	if(AppDataFile_GetSamplingEnabled())
	{
		samplingWasEnabled = TRUE;
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0");
	}

	if(FS_openFile(&sampleFile,fileNameSrc) != ERR_OK)
	{
		SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"PrintLiDoFile open failed");
		return ERR_FAILED;
	}

	while(FS_getLiDoSampleOutOfFile(&sampleFile,sampleBuf,LIDO_SAMPLE_SIZE,&nofReadChars) == ERR_OK &&
		  nofReadChars == LIDO_SAMPLE_SIZE)
	{
		sampleNr ++;

		unixTimeStamp = (int32)(sampleBuf[1] | sampleBuf[2]<<8 | sampleBuf[3]<<16 | sampleBuf[4]<<24);
		TmDt1_UnixSecondsToTimeDate(unixTimeStamp,0,&time,&date);

		UTIL1_strcpy(samplePrintLine,120,"#");
		UTIL1_strcatNum16uFormatted(samplePrintLine,120,sampleNr,'0',3);

		UTIL1_strcat(samplePrintLine,120," D: ");
		UTIL1_strcatNum16uFormatted(samplePrintLine, 120, date.Day, '0', 2);
		UTIL1_chcat(samplePrintLine, 120, '.');
		UTIL1_strcatNum16uFormatted(samplePrintLine, 120, date.Month, '0', 2);
		UTIL1_chcat(samplePrintLine, 120, '.');
		UTIL1_strcatNum16u(samplePrintLine, 120, (uint16_t)date.Year);

		UTIL1_strcat(samplePrintLine,120," T: ");
		UTIL1_strcatNum16sFormatted(samplePrintLine, 120, time.Hour, '0', 2);
		UTIL1_chcat(samplePrintLine, 120, ':');
		UTIL1_strcatNum16sFormatted(samplePrintLine, 120, time.Min, '0', 2);
		UTIL1_chcat(samplePrintLine, 120, ':');
		UTIL1_strcatNum16sFormatted(samplePrintLine, 120, time.Sec, '0', 2);

		UTIL1_strcat(samplePrintLine,120," L: x");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[5] | sampleBuf[6]<<8));
		UTIL1_strcat(samplePrintLine,120," y");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[7] | sampleBuf[8]<<8));
		UTIL1_strcat(samplePrintLine,120," z");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[9] | sampleBuf[10]<<8));
		UTIL1_strcat(samplePrintLine,120," ir");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[11] | sampleBuf[12]<<8));
		UTIL1_strcat(samplePrintLine,120," b");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[13] | sampleBuf[14]<<8));
		UTIL1_strcat(samplePrintLine,120," b");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[15] | sampleBuf[16]<<8));

		UTIL1_strcat(samplePrintLine,120," A: x");
		UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[17]);
		UTIL1_strcat(samplePrintLine,120," y");
		UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[18]);
		UTIL1_strcat(samplePrintLine,120," z");
		UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[19]);

		if(sampleBuf[20] & 0x80 )  //MarkerPresent!
		{
			UTIL1_strcat(samplePrintLine,120," T: ");
			UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[20] & ~0x80);
			UTIL1_strcat(samplePrintLine,120," M: true");
		}
		else
		{
			UTIL1_strcat(samplePrintLine,120," T: ");
			UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[20]);
			UTIL1_strcat(samplePrintLine,120," M: false");
		}

		UTIL1_strcat(samplePrintLine,120,"\r\n");
		CLS1_SendStr(samplePrintLine,io->stdErr);
	}

	if(FS_closeFile(&sampleFile) != ERR_OK)
	{
		SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
		return ERR_FAILED;
	}
	return ERR_OK;
}


uint8_t APP_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io)
{
  unsigned char fileName[48];
  size_t lenRead;
  uint8_t res = ERR_OK;
  const uint8_t *p;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "APP help")==0)
  {
    CLS1_SendHelpStr((unsigned char*)"APP", (const unsigned char*)"Group of Application commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  printFile <file>", (const unsigned char*)"Prints a LiDo Sample File\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  }
  else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LightSens status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  else if (UTIL1_strncmp((char* )cmd, "APP printFile ",	sizeof("APP printFile ") - 1)	== 0)
  {
	*handled = TRUE;
	if ((UTIL1_ReadEscapedName(cmd + sizeof("APP printFile ") - 1,fileName, sizeof(fileName), &lenRead, NULL, NULL)	== ERR_OK))
	{
		return PrintLiDoFile(fileName, io);
	}
	return ERR_FAILED;
  }
  return res;
}
