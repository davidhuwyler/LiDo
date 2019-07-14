/*
 * Application.c
 *
 *  Created on: 17.02.2018
 *      Author: Erich Styger
 */

#include "Platform.h"
#include "Application.h"
#include "FRTOS1.h"
#include "LED_R.h"
#include "LED_G.h"
#include "LED_B.h"
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
#include "TmDt1.h"
#include "WatchDog.h"
#include "SPIF.h"
#include "WDog1.h"
#include "PowerManagement.h"
#include "PIN_POWER_ON.h"
#include "LowPower.h"
#include "LightAutoGain.h"
#include "SYS1.h"
#include "WAIT1.h"
#include "CI2C1.h"
#include "I2C_SCL.h"
#include "I2C_SDA.h"
#include "PORT_PDD.h"
#include "GPIO_PDD.h"
#if PL_CONFIG_HAS_GAUGE_SENSOR
  #include "McuLC709203F.h"
#endif

#define MUTEX_WAIT_TIME_MS 2000

#define SAMPLE_THRESHOLD_TEMP 2 //The Temperature has to rise/fall more than this threshold to trigger a sample
#define SAMPLE_THRESHOLD_ACCEL 2 //The Acceleration has to rise/fall more than this threshold to trigger a sample

static TaskHandle_t sampletaskHandle;
static TaskHandle_t writeFileTaskHandle;
static SemaphoreHandle_t sampleMutex;
static SemaphoreHandle_t fileAccessMutex;

static QueueHandle_t lidoSamplesToWrite;
static lfs_file_t sampleFile;

static volatile bool fileIsOpen = FALSE;
static volatile bool setOneMarkerInLog = FALSE;
static volatile bool toggleEnablingSampling = FALSE;
static volatile bool requestForSoftwareReset = FALSE;
static volatile bool requestForPowerOff = FALSE;

void APP_FatalError(void) {
  for(;;) {
    LED_R_Neg();
    WAIT1_Waitms(100);
  }
}

void APP_setMarkerInLog(void) {
  setOneMarkerInLog = TRUE;
}

void APP_toggleEnableSampling(void) {
  toggleEnablingSampling = TRUE;
}

void APP_requestForSoftwareReset(void) {
  requestForSoftwareReset = TRUE;
}

void APP_requestForPowerOff(void) {
  requestForPowerOff = TRUE;
}

static void APP_toggleEnableSamplingIfRequested(void) {
  if(toggleEnablingSampling) {
    WatchDog_StartComputationTime(WatchDog_ToggleEnableSampling);
    toggleEnablingSampling = FALSE;
    if(AppDataFile_GetSamplingEnabled()) {
      AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0");
    } else {
      AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"1");
    }
    WatchDog_StopComputationTime(WatchDog_ToggleEnableSampling);
  }
}

static void APP_softwareResetIfRequested(void) {
  if(requestForSoftwareReset) {
    requestForSoftwareReset = FALSE;
    if (fileIsOpen) {
      FS_closeFile(&sampleFile);
    }
    KIN1_SoftwareReset();
  }
}

static void APP_PowerOffIfRequested(void) {
  if(requestForPowerOff) {
    requestForPowerOff = FALSE;
    if (fileIsOpen) {
      FS_closeFile(&sampleFile);
    }
    PowerManagement_PowerOff(); /* only effective if USB is not powering the board */
  }
}

int abs(int i) {
  return i < 0 ? -i : i;
}

uint8_t APP_getCurrentSample(liDoSample_t* sample, int32 unixTimestamp, bool forceSample) {
  static AccelAxis_t oldAccelAndTemp;
  LightChannels_t lightB0,lightB1;
  AccelAxis_t accelAndTemp;
  uint8_t err = ERR_OK;
  uint8_t lightGain, lightIntTime, lightWaitTime;

  if(xSemaphoreTakeRecursive(sampleMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS))) {
    WatchDog_StartComputationTime(WatchDog_TakeLidoSample);
    sample->unixTimeStamp = unixTimestamp;
#if PL_CONFIG_HAS_ACCEL_SENSOR
    if(AccelSensor_getValues(&accelAndTemp) != ERR_OK)
    {
      SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_ACCELSENS_ERROR, "Accel get values failed");
    }
    sample->accelX = accelAndTemp.xValue;
    sample->accelY = accelAndTemp.yValue;
    sample->accelZ = accelAndTemp.zValue;
#else
    sample->accelX = 0;
    sample->accelY = 0;
    sample->accelZ = 0;
#endif

    int8_t tempDiff = (int8_t)abs((int8_t)oldAccelAndTemp.temp-(int8_t)accelAndTemp.temp);
    int8_t xAccelDiff = (int8_t)abs((int8_t)oldAccelAndTemp.xValue-(int8_t)accelAndTemp.xValue) ;
    int8_t yAccelDiff = (int8_t)abs((int8_t)oldAccelAndTemp.yValue-(int8_t)accelAndTemp.yValue) ;
    int8_t zAccelDiff = (int8_t)abs((int8_t)oldAccelAndTemp.zValue-(int8_t)accelAndTemp.zValue) ;

    if (forceSample ||
      tempDiff > SAMPLE_THRESHOLD_TEMP ||
      xAccelDiff > SAMPLE_THRESHOLD_ACCEL ||
      yAccelDiff > SAMPLE_THRESHOLD_ACCEL ||
      zAccelDiff > SAMPLE_THRESHOLD_ACCEL
      )
    {
#if PL_CONFIG_HAS_LIGHT_SENSOR
      LightSensor_getParams(&lightGain,&lightIntTime,&lightWaitTime);
      sample->lightGain = lightGain;
      sample->lightIntTime = lightIntTime;
      if(LightSensor_getChannelValues(&lightB0,&lightB1) != ERR_OK)
      {
        SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_LIGHTSENS_ERROR, "LightSensor get values failed");
      }
      sample->lightChannelX = lightB1.xChannelValue;
      sample->lightChannelY = lightB1.yChannelValue;
      sample->lightChannelZ = lightB1.zChannelValue;
      sample->lightChannelIR = lightB1.nChannelValue;
      sample->lightChannelB440 = lightB0.nChannelValue;
      sample->lightChannelB490 = lightB0.zChannelValue;
#else
      sample->lightGain = 0;
      sample->lightIntTime = 0;
      sample->lightChannelX = 0;
      sample->lightChannelY = 0;
      sample->lightChannelZ = 0;
      sample->lightChannelIR = 0;
      sample->lightChannelB440 = 0;
      sample->lightChannelB490 = 0;
#endif
      if(setOneMarkerInLog) {
        setOneMarkerInLog =FALSE;
        sample->temp = accelAndTemp.temp | 0x80;
      } else {
        sample->temp = accelAndTemp.temp;
      }
      crc8_liDoSample(sample);
      WatchDog_StopComputationTime(WatchDog_TakeLidoSample);
      xSemaphoreGiveRecursive(sampleMutex);
      if(AppDataFile_GetAutoGainEnabled()) {
        uint8_t lightSensorIntTime, lightSensorGain;
        LiGain_Compute(sample,&lightSensorIntTime,&lightSensorGain);
#if PL_CONFIG_HAS_LIGHT_SENSOR
        LightSensor_setParams(lightSensorGain,lightSensorIntTime,lightSensorIntTime);
#endif
      }
      oldAccelAndTemp = accelAndTemp;
      return ERR_OK;
    } else {
      return ERR_DISABLED;
    }
  } else {
    return ERR_FAILED;
  }
}

static bool APP_newDay(void) {
   static DATEREC oldDate;
   DATEREC newDate;

   TmDt1_GetDate(&newDate);
   if(newDate.Day != oldDate.Day) {
     oldDate = newDate;
     return TRUE;
   }
   return FALSE;
}

static bool APP_newHour(void) {
   static TIMEREC oldTime;
   TIMEREC newTime;

   TmDt1_GetTime(&newTime);
   if(newTime.Hour != oldTime.Hour) {
     oldTime = newTime;
     return TRUE;
   }
   return FALSE;
}

void APP_resumeSampleTaskFromISR(void) {
  BaseType_t xYieldRequired;

  if(sampletaskHandle!=NULL) {
    xYieldRequired = xTaskResumeFromISR(sampletaskHandle); // Enable Sample Task for Execution
    if( xYieldRequired == pdTRUE ) {
      portYIELD_FROM_ISR(pdTRUE);
    }
  }
}

void APP_suspendSampleTask(void) {
  if(sampletaskHandle!=NULL) {
    vTaskSuspend(sampletaskHandle);
  } else {
    APP_FatalError();
  }
}

void APP_suspendWriteFileTask(void) {
  if(writeFileTaskHandle!=NULL) {
    vTaskSuspend(writeFileTaskHandle);
  } else {
    APP_FatalError();
  }
}

static void APP_sample_task(void *param) {
  uint8_t sampleError = ERR_OK;
  liDoSample_t sample;
  int32_t unixTScurrentSample;

  (void)param; /* not used */
  sampleMutex = xSemaphoreCreateRecursiveMutex();
  if (sampleMutex == NULL) {
    APP_FatalError();
  }
  xSemaphoreGiveRecursive(sampleMutex);
  for(;;) {
    WatchDog_StartComputationTime(WatchDog_MeasureTaskRuns);
    if(AppDataFile_GetSamplingEnabled()) {
      RTC_getTimeUnixFormat(&unixTScurrentSample);
      sampleError = APP_getCurrentSample(&sample,unixTScurrentSample, !AppDataFile_GetSampleAutoOff());
      if(sampleError == ERR_FAILED) {
        SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_SAMPLING_ERROR, "getCurrentSample failed");
      } else if(sampleError==ERR_OK) {
        UI_LEDpulse(LED_V);
        if(xQueueSendToBack(lidoSamplesToWrite, (void*)&sample, pdMS_TO_TICKS(500)) != pdPASS ) {
          SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_SAMPLING_ERROR, "lidoSamplesToWrite failed");
        }
      }
    }
    WatchDog_StopComputationTime(WatchDog_MeasureTaskRuns);
    WatchDog_ResumeTask();
    if(writeFileTaskHandle != NULL) {
      vTaskResume(writeFileTaskHandle);
    }
    PowerManagement_ResumeTaskIfNeeded();
    APP_suspendSampleTask(); /* will be woken up either by RTC alarm or LightSensor_Done_ISR() signal */
  } /* for */
}

static void APP_makeNewFileIfNeeded(void) {
  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
  //New Hour: Make new File!
  if(fileIsOpen && AppDataFile_GetSamplingEnabled() && APP_newHour()) {
    WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
    if(FS_closeFile(&sampleFile) != ERR_OK) {
      SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
    }
    WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
    WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
    if(FS_openLiDoSampleFile(&sampleFile) != ERR_OK) {
      SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_openLiDoSampleFile failed");
    }
    WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
  }
  xSemaphoreGiveRecursive(fileAccessMutex);
}

static void APP_openFileIfNeeded(void) {
  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
  if(AppDataFile_GetSamplingEnabled() && !fileIsOpen) {
    WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
    if(FS_openLiDoSampleFile(&sampleFile) == ERR_OK) {
      fileIsOpen = TRUE;
    } else {
      SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_openLiDoSampleFile failed");
    }
    WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
  }
  xSemaphoreGiveRecursive(fileAccessMutex);
}

static void APP_writeQueuedSamplesToFile(void) {
  liDoSample_t sample;
  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
  if(AppDataFile_GetSamplingEnabled() && fileIsOpen) {
    //Write all pending samples to file
    while(xQueuePeek(lidoSamplesToWrite, &sample, 0) == pdPASS) {
      WatchDog_StartComputationTime(WatchDog_WriteToLidoSampleFile);
      if(FS_writeLiDoSample(&sample,&sampleFile) != ERR_OK) {
        SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_writeLiDoSample failed");
      } else {
        xQueueReceive(lidoSamplesToWrite,&sample,pdMS_TO_TICKS(500));
      }
      WatchDog_StopComputationTime(WatchDog_WriteToLidoSampleFile);
    }
  } else if (fileIsOpen)  {
    xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
    WatchDog_StartComputationTime(WatchDog_OpenCloseLidoSampleFile);
    if(FS_closeFile(&sampleFile) == ERR_OK) {
      fileIsOpen = FALSE;
    } else {
      SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
    }
    WatchDog_StopComputationTime(WatchDog_OpenCloseLidoSampleFile);
  }
  xSemaphoreGiveRecursive(fileAccessMutex);
}

static void APP_writeLidoFile_task(void *param) {
  uint8_t samplingIntervall;

  (void)param; /* not used */
  fileAccessMutex = xSemaphoreCreateRecursiveMutex();
  if( fileAccessMutex == NULL ) {
    APP_FatalError();
  }
  xSemaphoreGiveRecursive(fileAccessMutex);
  for(;;) {
    if(LowPower_StopModeIsEnabled()) {
      SPIF_ReleaseFromDeepPowerDown();
    }
    APP_softwareResetIfRequested();
    APP_PowerOffIfRequested();
    APP_toggleEnableSamplingIfRequested();
    AppDataFile_GetSampleIntervall(&samplingIntervall);
    APP_makeNewFileIfNeeded();
    APP_openFileIfNeeded();
    APP_writeQueuedSamplesToFile();
    APP_suspendWriteFileTask();
    if(LowPower_StopModeIsEnabled()) {
      SPIF_GoIntoDeepPowerDown();
    }
  } /* for */
}

static void APP_init(void) {
  //Init the SampleQueue, SampleTask and the WriteLidoFile Task
  //The SampleQueue is used to transfer Samples SampleTask-->WriteLidoFile
  lidoSamplesToWrite = xQueueCreate(15, sizeof(liDoSample_t));
  if( lidoSamplesToWrite == NULL) {
    APP_FatalError();
  }
  if (xTaskCreate(APP_sample_task, "sampleTask", 1500/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+3, &sampletaskHandle) != pdPASS) {
    APP_FatalError();
  }
  RTC_InitRTCInterrupt();
  if (xTaskCreate(APP_writeLidoFile_task, "lidoFileWriter", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2, &writeFileTaskHandle) != pdPASS) {
    APP_FatalError();
  }
}

static bool APP_WaitIfButtonPressed3s(void) {
  if(USER_BUTTON_PRESSED()) {
    for(int i = 0 ; i < 30 ; i++) {
      WDog1_Clear();
      vTaskDelay(pdMS_TO_TICKS(100));
      LED_R_Neg();
      if(!USER_BUTTON_PRESSED()) {
        return FALSE;
      }
    }
    return TRUE;
  } else {
    return FALSE;
  }
}


static void MuxAsGPIO(void) {
  /* PTB3: SDA, PTB2: SCL */
  //CI2C1_Deinit(NULL);
  I2C_SDA_ConnectPin(); /* mux as GPIO */
  I2C_SCL_ConnectPin(); /* mux as GPOO */
  I2C_SDA_SetOutput();
  I2C_SCL_SetOutput();
}

static void MuxAsI2C(void) {
  /* PTB3: SDA, PTB2: SCL */
  //CI2C1_Init(NULL);
  /* mux back to normal I2C mode with interrupts enabled */
  /* PCR3: SDA */
  PORTB_PCR3 = (uint32_t)((PORTB_PCR3 & (uint32_t)~(uint32_t)(
                PORT_PCR_ISF_MASK |
                PORT_PCR_MUX(0x05)
               )) | (uint32_t)(
                PORT_PCR_MUX(0x02)
               ));
  PORT_PDD_SetPinOpenDrain(PORTB_BASE_PTR, 0x03u, PORT_PDD_OPEN_DRAIN_ENABLE); /* Set SDA pin as open drain */
  /* PORTB_PCR2: ISF=0,MUX=2 */
  PORTB_PCR2 = (uint32_t)((PORTB_PCR2 & (uint32_t)~(uint32_t)(
                PORT_PCR_ISF_MASK |
                PORT_PCR_MUX(0x05)
               )) | (uint32_t)(
                PORT_PCR_MUX(0x02)
               ));
  PORT_PDD_SetPinOpenDrain(PORTB_BASE_PTR, 0x02u, PORT_PDD_OPEN_DRAIN_ENABLE); /* Set SCL pin as open drain */
}

static void ResetI2CBus(void) {
  int i;
  MuxAsGPIO();

  /* Drive SDA low first to simulate a start */
  I2C_SDA_ClrVal();//McuGPIO_Low(sdaPin);
  WAIT1_Waitus(10);
  /* Send 9 pulses on SCL and keep SDA high */
  for (i = 0; i < 9; i++) {
    I2C_SCL_ClrVal();//McuGPIO_Low(sclPin);
    WAIT1_Waitus(10);

    I2C_SDA_SetVal();//McuGPIO_High(sdaPin);
    WAIT1_Waitus(10);

    I2C_SCL_SetVal();//McuGPIO_High(sclPin);
    WAIT1_Waitus(20);
  }
  /* Send stop */
  I2C_SCL_ClrVal();//McuGPIO_Low(sclPin);
  WAIT1_Waitus(10);
  I2C_SDA_ClrVal();//McuGPIO_Low(sdaPin);
  WAIT1_Waitus(10);

  I2C_SCL_ClrVal();//McuGPIO_Low(sclPin);
  WAIT1_Waitus(10);

  I2C_SDA_SetVal();//McuGPIO_High(sdaPin);
  WAIT1_Waitus(10);
  /* go back to I2C mode */
  MuxAsI2C();
}

static void APP_init_task(void *param) {
  (void)param; /* not used */
  //LED_G_On();
  //vTaskDelay(pdMS_TO_TICKS(200));
  //LED_G_Off();

  if(!APP_WaitIfButtonPressed3s()) { /* Normal init if the UserButton is not pressed */
#if PL_CONFIG_HAS_GAUGE_SENSOR
    McuLC_Wakeup(); /* needs to be done before (!!!) any other I2C communication! */
#endif
    ResetI2CBus();
#if PL_CONFIG_HAS_GAUGE_SENSOR
    McuLC_Wakeup(); /* need to do the wakeup again after the reset bus? otherwise will be stuck in McuLC_Init() */
    McuLC_Init();
#endif
    WatchDog_Init();
    WatchDog_StartComputationTime(WatchDog_LiDoInit);
    SDEP_Init();
    FS_Init();
    AppDataFile_Init();
    SHELL_Init();
    LowPower_init();
#if PL_CONFIG_HAS_LIGHT_SENSOR
    LightSensor_init();
#endif
#if PL_CONFIG_HAS_ACCEL_SENSOR
    AccelSensor_init();
#endif
    if(RCM_SRS0 & RCM_SRS0_POR_MASK) { // Init from PowerOn Reset
      AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0"); //Disable Sampling
      RTC_init(FALSE);   /* HardReset RTC */
    } else {
      RTC_init(TRUE); /* softreset RTC */
    }
    UI_Init();
    PowerManagement_init();
    APP_init();
    WatchDog_StopComputationTime(WatchDog_LiDoInit);
  } else { //Format SPIF after 9s ButtonPress after startup
#if 0 /*! \todo Disabled as not safe! */
    if(USER_BUTTON_PRESSED()) {
      LED_R_Off();
      vTaskDelay(pdMS_TO_TICKS(3000));
      if(APP_WaitIfButtonPressed3s()) //Format SPIF
      {
        RTC_init(FALSE);    //HardReset RTC
        FS_FormatInit();  //Format FS after 9s ButtonPress
        AppDataFile_Init();
        AppDataFile_CreateFile();
      }
      LED_R_Off();
    } else {
      FS_Init();
      AppDataFile_Init();
    }
    KIN1_SoftwareReset();
#endif
  }
  vTaskDelete(NULL); /* kill own task as not needed any more */
}

void APP_CloseSampleFile(void) {
  xSemaphoreTakeRecursive(fileAccessMutex,pdMS_TO_TICKS(MUTEX_WAIT_TIME_MS));
  if(fileIsOpen) {
    if(FS_closeFile(&sampleFile) != ERR_OK) {
        SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
    } else {
      fileIsOpen = FALSE;
    }
  }
  xSemaphoreGiveRecursive(fileAccessMutex);
}

#if 0
static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"App", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}
#endif

static uint8_t PrintLiDoFile(uint8_t* fileNameSrc, CLS1_ConstStdIOType *io) {
  lfs_file_t sampleFile;
  uint8_t nofReadChars;
  bool samplingWasEnabled = FALSE;
  uint8_t sampleBuf[LIDO_SAMPLE_SIZE];
  uint8_t samplePrintLine[120];
  uint16_t sampleNr = 0;
  int32 unixTimeStamp;
  TIMEREC time;
  DATEREC date;

  if(AppDataFile_GetSamplingEnabled()) {
    samplingWasEnabled = TRUE;
    AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],"0");
  }
  if(FS_openFile(&sampleFile,fileNameSrc) != ERR_OK) {
    SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"PrintLiDoFile open failed");
    return ERR_FAILED;
  }
  WatchDog_DisableSource(WatchDog_MeasureTaskRuns);
  //Read + Print Header:
  if(FS_readLine(&sampleFile,samplePrintLine,120,&nofReadChars) != ERR_OK) {
    SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"PrintLiDoFile read Header failed");
    return ERR_FAILED;
  }

  CLS1_SendStr("\r\n",io->stdOut);
  CLS1_SendStr(samplePrintLine,io->stdOut);
  CLS1_SendStr("\r\n",io->stdOut);

  //Read + Print Samples
  while(   FS_getLiDoSampleOutOfFile(&sampleFile,sampleBuf,LIDO_SAMPLE_SIZE,&nofReadChars) == ERR_OK
        && nofReadChars == LIDO_SAMPLE_SIZE)
  {
    sampleNr++;

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

    UTIL1_strcat(samplePrintLine,120," P: G");
    UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[4]);
    UTIL1_strcat(samplePrintLine,120," I");
    UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[5]);

    UTIL1_strcat(samplePrintLine,120," L: x");
    UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[6] | sampleBuf[7]<<8));
    UTIL1_strcat(samplePrintLine,120," y");
    UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[8] | sampleBuf[9]<<8));
    UTIL1_strcat(samplePrintLine,120," z");
    UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[10] | sampleBuf[11]<<8));
    UTIL1_strcat(samplePrintLine,120," ir");
    UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[12] | sampleBuf[13]<<8));
    UTIL1_strcat(samplePrintLine,120," b");
    UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[14] | sampleBuf[15]<<8));
    UTIL1_strcat(samplePrintLine,120," b");
    UTIL1_strcatNum16u(samplePrintLine, 120,(uint16_t)(sampleBuf[16] | sampleBuf[17]<<8));

    UTIL1_strcat(samplePrintLine,120," A: x");
    UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[18]);
    UTIL1_strcat(samplePrintLine,120," y");
    UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[19]);
    UTIL1_strcat(samplePrintLine,120," z");
    UTIL1_strcatNum8s(samplePrintLine, 120,sampleBuf[20]);

    if(sampleBuf[21] & 0x80) { //MarkerPresent!
      UTIL1_strcat(samplePrintLine,120," T: ");
      UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[21] & ~0x80);
      UTIL1_strcat(samplePrintLine,120," M: true");
    } else {
      UTIL1_strcat(samplePrintLine,120," T: ");
      UTIL1_strcatNum8u(samplePrintLine, 120,sampleBuf[21]);
      UTIL1_strcat(samplePrintLine,120," M: false");
    }
    UTIL1_strcat(samplePrintLine,120,"\r\n");
    CLS1_SendStr(samplePrintLine,io->stdErr);
  }

  if(FS_closeFile(&sampleFile) != ERR_OK) {
    SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_STORAGE_ERROR,"FS_closeLiDoSampleFile failed");
    return ERR_FAILED;
  }
  WatchDog_EnableSource(WatchDog_MeasureTaskRuns);
  return ERR_OK;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32], res;
  LightChannels_t bank0, bank1;

  CLS1_SendStatusStr((unsigned char*)"App", (const unsigned char*)"\r\n", io->stdOut);
  UTIL1_strcpy(buf, sizeof(buf), (const unsigned char*)"V");
  UTIL1_strcatNum16u(buf, sizeof(buf), PL_BOARD_REV/10);
  UTIL1_chcat(buf, sizeof(buf), '.');
  UTIL1_strcatNum16u(buf, sizeof(buf), PL_BOARD_REV%10);
  UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"\r\n");
  CLS1_SendStatusStr((unsigned char*)"  Board Rev.", (unsigned char *)buf, io->stdOut);
  CLS1_SendStatusStr((unsigned char*)"  Sampling", AppDataFile_GetSamplingEnabled()?(unsigned char *)"yes\r\n":(unsigned char *)"no\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t APP_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  unsigned char fileName[48];
  size_t lenRead;
  uint8_t res = ERR_OK;
  const uint8_t *p;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "App help")==0) {
    CLS1_SendHelpStr((unsigned char*)"App", (const unsigned char*)"Group of Application commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  print <file>", (const unsigned char*)"Prints a LiDo Sample File\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  } else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "App status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  } else if (UTIL1_strncmp((char* )cmd, "App print ", sizeof("App print ") - 1) == 0) {
    *handled = TRUE;
    if ((UTIL1_ReadEscapedName(cmd + sizeof("App print ") - 1,fileName, sizeof(fileName), &lenRead, NULL, NULL) == ERR_OK)) {
      return PrintLiDoFile(fileName, io);
    }
    return ERR_FAILED;
  }
  return res;
}

void APP_Run(void) {
  PowerManagement_PowerOn(); /* turn on FET to keep Vcc supplied. should already be done by default during PE_low_level_init() */
#if 0
  //EmercencyBreak: If LowPower went wrong...
  while(USER_BUTTON_PRESSED()) {
    LED_R_Neg();
    WAIT1_Waitms(50);
  }
#endif
#if 1
  for (int i=0; i<4; i++) {
    LED_G_Neg();
    LED_B_Neg();
    LED_R_Neg();
    WAIT1_Waitms(100);
  }
#endif
#if 0
  for (int i=0;i<20;i++) {
    LED_R_Neg();
    WAIT1_Waitms(500);
  }
  PIN_POWER_ON_ClrVal(); /* power off */
#endif
  if (xTaskCreate(APP_init_task, "Init", 1500/sizeof(StackType_t), NULL, configMAX_PRIORITIES-1, NULL) != pdPASS) {
    APP_FatalError(); /* error! probably out of memory */
  }
  vTaskStartScheduler();
  for(;;) {} /* should not get here */
}
