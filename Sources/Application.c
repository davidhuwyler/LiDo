/*
 * Application.c
 *
 *  Created on: 17.02.2018
 *      Author: Erich Styger
 */

#include "Application.h"
#include "FRTOS1.h"
#include "LED_R.h"
#include "LED_G.h"
#include "WAIT1.h"
#include "KIN1.h"
#include "LightSensor.h"
#include "AccelSensor.h"
#include "Shell.h"
#include "RTC.h"
#include "FileSystem.h"
#include "UI.h"
#include "SDEP.h"
#include "AppDataFile.h"
#include "CRC8.h"
#include "RTC1.h"
#include "TmDt1.h"
#include "WatchDog.h"
#include "SPIF.h"
#include "WDog1.h"
#include "PowerManagement.h"
#include "PIN_POWER_ON.h"
#include "LowPower.h"

#define MUTEX_WAIT_TIME_MS 2000
static TaskHandle_t sampletaskHandle;
static SemaphoreHandle_t sampleMutex;
static SemaphoreHandle_t fileAccessMutex;
static QueueHandle_t lidoSamplesToWrite;
static lfs_file_t sampleFile;

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

static void APP_softwareResetIfRequested()
{
	taskENTER_CRITICAL();
	if(requestForSoftwareReset)
	{
		requestForSoftwareReset = FALSE;
		taskEXIT_CRITICAL();
		if(fileIsOpen)
		{
			FS_closeFile(&sampleFile);
		}
		//TODO deinit Stuff...
		KIN1_SoftwareReset();
	}
	else
	{
		taskEXIT_CRITICAL();
	}
}

uint8_t APP_getCurrentSample(liDoSample_t* sample, int32 unixTimestamp)
{
	  LightChannels_t lightB0,lightB1;
	  AccelAxis_t accelAndTemp;
	  uint8_t err = ERR_OK;

	  if(xSemaphoreTakeRecursive(sampleMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS)))
	  {
		  WatchDog_StartComputationTime(WatchDog_TakeLidoSample);
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
		  xSemaphoreGiveRecursive(sampleMutex);
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

void APP_resumeSampleTaskFromISR(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	BaseType_t xYieldRequired;
	if(sampletaskHandle!=NULL)
	{
	    xYieldRequired = xTaskResumeFromISR( sampletaskHandle );//Enable Sample Task for Execution
	    if( xYieldRequired == pdTRUE )
	    {
	        portYIELD_FROM_ISR(pdTRUE);
	    }
	}
    CS1_ExitCritical();
}

void APP_suspendSampleTask(void)
{
	if(sampletaskHandle!=NULL)
	{
		vTaskSuspend(sampletaskHandle);
	}
	else
	{
		//Error
		for(;;)
		{
			LED_R_Neg();
			WAIT1_Waitms(50);
		}
	}

}

static void APP_sample_task(void *param) {
  (void)param;
  liDoSample_t sample;
  static int32_t unixTSlastSample;
  int32_t unixTScurrentSample;
  sampleMutex = xSemaphoreCreateRecursiveMutex();
  xSemaphoreGiveRecursive(sampleMutex);
  for(;;)
  {
	  WatchDog_StartComputationTime(WatchDog_MeasureTaskRunns);
	  if(AppDataFile_GetSamplingEnabled())
	  {
		  UI_LEDpulse(LED_V);
		  RTC_getTimeUnixFormat(&unixTScurrentSample);
		  if(APP_getCurrentSample(&sample,unixTScurrentSample) != ERR_OK)
		  {
			  SDEP_InitiateNewAlert(SDEP_ALERT_SAMPLING_ERROR);
		  }

	      if( xQueueSendToBack( lidoSamplesToWrite,  ( void * ) &sample, pdMS_TO_TICKS(500)) != pdPASS )
	      {
	    	  SDEP_InitiateNewAlert(SDEP_ALERT_SAMPLING_ERROR);
	      }
	  }
	  WatchDog_StopComputationTime(WatchDog_MeasureTaskRunns);

	  //Debug
	  vTaskDelay(1000);

	  //APP_suspendSampleTask();
  } /* for */
}

static void APP_makeNewFileIfNeeded()
{
	  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
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
	  xSemaphoreGiveRecursive(fileAccessMutex);
}

static void APP_openFileIfNeeded()
{
	  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
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
	  xSemaphoreGiveRecursive(fileAccessMutex);
}

static void APP_writeQueuedSamplesToFile()
{
	  liDoSample_t sample;
	  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
	  if(AppDataFile_GetSamplingEnabled() && fileIsOpen)
	  {

		  //Write all pending samples to file
		  while(xQueuePeek(lidoSamplesToWrite,&sample,pdMS_TO_TICKS(500)) == pdPASS)
		  {
			  WatchDog_StartComputationTime(WatchDog_WriteToLidoSampleFile);
			  if(FS_writeLiDoSample(&sample,&sampleFile) != ERR_OK)
			  {
				  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_writeLiDoSample failed");
			  }
			  else
			  {
				  xQueueReceive(lidoSamplesToWrite,&sample,pdMS_TO_TICKS(500));
			  }
			  WatchDog_StopComputationTime(WatchDog_WriteToLidoSampleFile);
		  }
	  }
	  else if (fileIsOpen)
	  {
		  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
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
	  xSemaphoreGiveRecursive(fileAccessMutex);
}

static void APP_writeLidoFile_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  uint8_t samplingIntervall;
  fileAccessMutex = xSemaphoreCreateRecursiveMutex();
  if( fileAccessMutex == NULL )
  {
  	for(;;){} /* error! probably out of memory */
  }
  xSemaphoreGiveRecursive(fileAccessMutex);

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  //SPIF_ReleaseFromDeepPowerDown();
	  APP_softwareResetIfRequested();
	  APP_toggleEnableSamplingIfRequested();

	  AppDataFile_GetSampleIntervall(&samplingIntervall);

	  APP_makeNewFileIfNeeded();
	  APP_openFileIfNeeded();
	  APP_writeQueuedSamplesToFile();

	  //SPIF_GoIntoDeepPowerDown();

	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(samplingIntervall*1000));
  } /* for */
}

void APP_init(void)
{
	//Init the SampleQueue, SampleTask and the WriteLidoFile Task
	//The SampleQueue is used to transfer Samples SampleTask-->WriteLidoFile
	lidoSamplesToWrite = xQueueCreate( 16, sizeof( liDoSample_t ) );
    if( lidoSamplesToWrite == NULL )
    {
    	for(;;){} /* error! probably out of memory */
    }

	if (xTaskCreate(APP_sample_task, "sampleTask", 1000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+3, &sampletaskHandle) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	//Init the RTC alarm Interrupt:
	//Debug
//	RTC_CR  |= RTC_CR_SUP_MASK; 	//Write to RTC Registers enabled
//	RTC_IER |= RTC_IER_TAIE_MASK; 	//Enable RTC Alarm Interrupt
//	RTC_IER |= RTC_IER_TOIE_MASK;	//Enable RTC Overflow Interrupt
//	RTC_IER |= RTC_IER_TIIE_MASK;	//Enable RTC Invalid Interrupt
//	RTC_TAR = RTC_TSR;				//RTC Alarm at RTC Time

	if (xTaskCreate(APP_writeLidoFile_task, "lidoFileWriter", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}
}

static bool APP_WaitIfButtonPressed3s(void)
{
	  if(USER_BUTTON_PRESSED)
	  {
		  for(int i = 0 ; i < 30 ; i++)
		  {
			  WDog1_Clear();
			  WAIT1_Waitms(100);
			  LED_R_Neg();
			  if(!USER_BUTTON_PRESSED)
			  {
				  return FALSE;
			  }
		  }
		  return TRUE;
	  }
	  else
	  {
		  return FALSE;
	  }

}

static void APP_init_task(void *param) {
  (void)param;
  LED_G_On();
  WAIT1_Waitms(1000);
  LED_G_Off();
  if(!APP_WaitIfButtonPressed3s() && !(RCM_SRS0 & RCM_SRS0_POR_MASK)) //Normal init if the UserButton is not pressed and no PowerOn reset
  {
		SDEP_Init();
		LightSensor_init();
		AccelSensor_init();
		FS_Init();
		AppDataFile_Init();
		SHELL_Init();
		LowPower_init();

		//Init LightSensor Params from AppDataFileS
		uint8_t headerBuf[5];
		const unsigned char *p;
		p = headerBuf;
		uint8_t gain = 0, intTime = 0, waitTime = 0;
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[5][0], (uint8_t*)p ,25); //Read LightSens Gain
		UTIL1_ScanDecimal8uNumber(&p, &gain);
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[6][0], (uint8_t*)p ,25); //Read LightSens IntegrationTime
		UTIL1_ScanDecimal8uNumber(&p, &intTime);
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[7][0], (uint8_t*)p ,25); //Read LightSens WaitTime
		UTIL1_ScanDecimal8uNumber(&p, &waitTime);
		LightSensor_setParams(gain,intTime,waitTime);

		WatchDog_Init();
		RTC_init(TRUE);
		UI_Init();
		PowerManagement_init();
		APP_init();
  }
  else //Init With HardReset RTC
  {
		RTC_init(FALSE);		//HardReset RTC
		if(USER_BUTTON_PRESSED)
		{
			LED_R_Off();
			WAIT1_Waitms(3000);
			if(APP_WaitIfButtonPressed3s()) //Format SPIF
			{
				FS_FormatInit();	//Format FS after 9s ButtonPress
				AppDataFile_Init();
				AppDataFile_CreateFile();
			}
			LED_R_Off();
		}
		else
		{
			FS_Init();
			AppDataFile_Init();
			AppDataFile_SetSamplingEnables(FALSE);
		}
		KIN1_SoftwareReset();
  }
  vTaskSuspend(xTaskGetCurrentTaskHandle());
}

void APP_CloseSampleFile(void)
{
	xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
	if(fileIsOpen)
	{
		if(FS_closeFile(&sampleFile) != ERR_OK)
		{
			  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
		}
		else
		{
			fileIsOpen = FALSE;
		}
	}
	xSemaphoreGiveRecursive(fileAccessMutex);
}

void APP_Run(void) {
	//PowerON
	PIN_POWER_ON_SetVal();

	//EmercencyBreak: If LowPower went wrong...
//	while(USER_BUTTON_PRESSED)
//	{
//		LED_R_Neg();
//		WAIT1_Waitms(50);
//	}

	if (xTaskCreate(APP_init_task, "Init", 1500/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
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

	WatchDog_DisableSource(WatchDog_MeasureTaskRunns);

	//Read + Print Header:
	if(FS_readLine(&sampleFile,samplePrintLine,120,&nofReadChars) != ERR_OK)
	{
		SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"PrintLiDoFile read Header failed");
		return ERR_FAILED;
	}

	CLS1_SendStr("\r\n",io->stdErr);
	CLS1_SendStr(samplePrintLine,io->stdErr);
	CLS1_SendStr("\r\n",io->stdErr);

	//Read + Print Samples
	while(FS_getLiDoSampleOutOfFile(&sampleFile,sampleBuf,LIDO_SAMPLE_SIZE,&nofReadChars) == ERR_OK &&
		  nofReadChars == LIDO_SAMPLE_SIZE)
	{
		sampleNr ++;

		unixTimeStamp = (int32)(sampleBuf[0] | sampleBuf[1]<<8 | sampleBuf[2]<<16 | sampleBuf[3]<<24);
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
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[4] | sampleBuf[5]<<8));
		UTIL1_strcat(samplePrintLine,120," y");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[6] | sampleBuf[7]<<8));
		UTIL1_strcat(samplePrintLine,120," z");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[8] | sampleBuf[9]<<8));
		UTIL1_strcat(samplePrintLine,120," ir");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[10] | sampleBuf[11]<<8));
		UTIL1_strcat(samplePrintLine,120," b");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[12] | sampleBuf[13]<<8));
		UTIL1_strcat(samplePrintLine,120," b");
		UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[14] | sampleBuf[15]<<8));

		UTIL1_strcat(samplePrintLine,120," A: x");
		UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[16]);
		UTIL1_strcat(samplePrintLine,120," y");
		UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[17]);
		UTIL1_strcat(samplePrintLine,120," z");
		UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[18]);

		if(sampleBuf[19] & 0x80 )  //MarkerPresent!
		{
			UTIL1_strcat(samplePrintLine,120," T: ");
			UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[19] & ~0x80);
			UTIL1_strcat(samplePrintLine,120," M: true");
		}
		else
		{
			UTIL1_strcat(samplePrintLine,120," T: ");
			UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[19]);
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

	WatchDog_EnableSource(WatchDog_MeasureTaskRunns);
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

void RTC_ALARM_ISR(void)
{
	if(RTC_SR & RTC_SR_TIF_MASK)/* Timer invalid (Vbat POR or RTC SW reset)? */
	{
		RTC_SR &= ~RTC_SR_TCE_MASK;  /* Disable counter */
		RTC_TPR = 0x00U;			 /* Reset prescaler */
		RTC_TSR = 0x02UL;			 /* Set init. time - 2000-01-01 0:0:1 (clears flag)*/
	}
	else if(RTC_SR & RTC_SR_TOF_MASK)
	{
		RTC_SR &= ~RTC_SR_TCE_MASK;  /* Disable counter */
		RTC_TPR = 0x00U;			 /* Reset prescaler */
		RTC_TSR = 0x02UL;			 /* Set init. time - 2000-01-01 0:0:1 (clears flag)*/
	}
	else /* Alarm interrupt */
	{
		uint8_t sampleIntervall;
		AppDataFile_GetSampleIntervall(&sampleIntervall);
		RTC_TAR = RTC_TSR + sampleIntervall - 1 ; 		//SetNext RTC Alarm
		APP_resumeSampleTaskFromISR();
	}

	  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
	     exception return operation might vector to incorrect interrupt */
	__DSB();
}
