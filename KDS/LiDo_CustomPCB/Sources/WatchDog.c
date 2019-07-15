/*
 * WatchDog.c
 *
 *  Created on: Mar 16, 2019
 *      Author: dave
 *
 *      Good watchdog reference: http://www.ganssle.com/watchdogs.htm
 */
#include "Platform.h"
#include "WatchDog.h"
#include "KIN1.h"
#include "FRTOS1.h"
#include "Application.h"
#include "AppDataFile.h"
#include "SDEP.h"
#include "WDog1.h"
#include "LowPower.h"
#include "LED_R.h"
#if PL_CONFIG_HAS_SENSOR_PWR_PIN
  #include "PIN_SENSOR_PWR.h"
#endif
#if PL_CONFIG_HAS_SPIF_PWR_PIN
  #include "PIN_SPIF_PWR.h"
#endif

#define FILE_OPEN_CLOSE_WRITE_MAX_DURATION_MS 5000

static TaskHandle_t watchDogTaskHandle;

typedef struct
{
	bool		isSingleCheckWatchdogSouce;		//If true, the WatchdogSource needs to be activated every Check and KickIntervall is not checked
	bool 		sourceIsActive;					//Flags an active WatchdogSource
	bool		compTimeMeasurementRunning;	    //Flags an active WatchdogSource Time Measurement
	bool		requestForDeactivation;			//Variable is checked if a source wants to deactivate its watchdog
	uint16_t	measuredCompTime;				//Measured Computation Time in ms
	TickType_t	timeStampLastKick;				//Time since last WatchDog kick from this Source in ms
	uint16_t 	lowerCompTimeLimit;				//Lower Boundary of allowed Computation time
	uint16_t 	uppwerCompTimeLimit;			//Upper Boundary of allowed Computation time									(no effect if isSingleCheckWatchdogSouce = true)
	uint16_t	maxKickIntervallLimitRaw;		//Upper Boundary of allowed Time between kicks 									(no effect if isSingleCheckWatchdogSouce = true)
	bool		kickIntervallXSampleIntervall;	//If true, the maxKickIntervallLimit is multiplied with the Lido Sample intervall (no effect if isSingleCheckWatchdogSouce = true)
	uint16_t	maxKickIntervallLimit;			//maxKickIntervallLimitRaw x SampleIntervall
	bool		sourceForceDisabled;			//Used to Disable a watchdogSource
} watchDogKickSource_t;

static watchDogKickSource_t watchDogKickSources[WatchDog_NOF_KickSources];

static void WatchDog_Task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  bool feedTheDog = TRUE;
  int i;

  for(;;) {
	  xLastWakeTime = xTaskGetTickCount();
	  taskENTER_CRITICAL();
		for(i=0; i<WatchDog_NOF_KickSources; i++) {
			if(watchDogKickSources[i].sourceIsActive)	{
				//Check Watchdog Kick Intervall
				if(!watchDogKickSources[i].isSingleCheckWatchdogSouce &&
					(xLastWakeTime - watchDogKickSources[i].timeStampLastKick) > watchDogKickSources[i].maxKickIntervallLimit)	//FeedDog interval ran out?
				{
					feedTheDog = FALSE; // --> WatchDog Source ran out... Reset!
					break;
				}

				//Check ComputationTime
				if(watchDogKickSources[i].compTimeMeasurementRunning)
				{ //If measurement Runs, only check upper Boundary
					if((xLastWakeTime - watchDogKickSources[i].timeStampLastKick) > watchDogKickSources[i].uppwerCompTimeLimit) {
						feedTheDog = FALSE; // --> WatchDog CompTime not in boundary... Reset!
						break;
					}
				}	else {
					 if(watchDogKickSources[i].measuredCompTime < watchDogKickSources[i].lowerCompTimeLimit || //ComputationTime of the Source in boundary?
							watchDogKickSources[i].measuredCompTime > watchDogKickSources[i].uppwerCompTimeLimit )
					{
						feedTheDog = FALSE; // --> WatchDog CompTime not in boundary... Reset!
						break;
					}
				}
				if(watchDogKickSources[i].requestForDeactivation)	{
					watchDogKickSources[i].sourceIsActive = FALSE;
				}
			}
		} /* for */
		taskEXIT_CRITICAL();

		if(!feedTheDog) { //Reset!
			//TODO Power Off SPIF und Sensoren
			WDog1_Clear();
    #if PL_CONFIG_HAS_SENSOR_PWR_PIN
			PIN_SENSOR_PWR_SetVal(); //PowerOff Sensors
    #endif
			//Send SDEP Alarm and Log Watchdog Reset:
			switch(i)
			{
				case WatchDog_OpenCloseLidoSampleFile:
					SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET, "WatchDogReset at OpenCloseLidoSampleFile");
					break;
				case WatchDog_WriteToLidoSampleFile:
					SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET, "WatchDogReset at WriteToLidoSampleFile");
					break;
				case WatchDog_TakeLidoSample:
					SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET, "WatchDogReset at TakeLidoSample");
					break;
				case WatchDog_ToggleEnableSampling:
					SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET, "WatchDogReset at ToggleEnableSampling");
					break;
				case WatchDog_MeasureTaskRuns:
					SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET, "WatchDogReset at MeasureTaskRunns");
					break;
				default:
				  SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET, "unknown WatchdogReset");
					break;
			}
#if PL_CONFIG_HAS_SPIF_PWR_PIN
			PIN_SPIF_PWR_SetVal(); //PowerOff SPIF
#endif
#if PL_CONFIG_HAS_WATCHDOG
			for(;;) {
			  APP_FatalError(__FILE__, __LINE__);
			}
#else
      #warning "watchdog is disabled!"
      for(;;) {
        WDog1_Clear();
        LED_R_Neg();
        vTaskDelay(pdMS_TO_TICKS(100));
      }
#endif
		}	else {
			WDog1_Clear();
		}

		if(LowPower_StopModeIsEnabled()) {
			vTaskSuspend(watchDogTaskHandle);
		}	else {
			vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(WATCHDOG_TASK_DELAY));
		}
  } /* for */
}

//Only in StopMode Needed
void WatchDog_ResumeTask(void) {
	if(LowPower_StopModeIsEnabled() && watchDogTaskHandle!=NULL) {
		vTaskResume(watchDogTaskHandle);
	}
}

void WatchDog_Init(void) {
#if PL_CONFIG_HAS_WATCHDOG
	WDog1_Enable();
	WDog1_Clear();
#endif
	//initial Dog feed...
	watchDogKickSources[WatchDog_LiDoInit].isSingleCheckWatchdogSouce	= TRUE;
	watchDogKickSources[WatchDog_LiDoInit].lowerCompTimeLimit 				= 0;
	watchDogKickSources[WatchDog_LiDoInit].uppwerCompTimeLimit 				= 5000;
	watchDogKickSources[WatchDog_LiDoInit].measuredCompTime 					= watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].lowerCompTimeLimit;
	watchDogKickSources[WatchDog_LiDoInit].timeStampLastKick 					= 0;
	watchDogKickSources[WatchDog_LiDoInit].sourceIsActive 						= FALSE;
	watchDogKickSources[WatchDog_LiDoInit].requestForDeactivation			= FALSE;
	watchDogKickSources[WatchDog_LiDoInit].sourceForceDisabled				= FALSE;

	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].isSingleCheckWatchdogSouce	= TRUE;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].lowerCompTimeLimit 			    = 0;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].uppwerCompTimeLimit 		    = FILE_OPEN_CLOSE_WRITE_MAX_DURATION_MS;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].measuredCompTime 				    = watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].lowerCompTimeLimit;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].timeStampLastKick 			    = 0;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].sourceIsActive 				      = FALSE;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].requestForDeactivation	    = FALSE;
	watchDogKickSources[WatchDog_OpenCloseLidoSampleFile].sourceForceDisabled			    = FALSE;

	watchDogKickSources[WatchDog_WriteToLidoSampleFile].isSingleCheckWatchdogSouce= TRUE;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].lowerCompTimeLimit 				= 0;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].uppwerCompTimeLimit 			= FILE_OPEN_CLOSE_WRITE_MAX_DURATION_MS;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].measuredCompTime 				  = watchDogKickSources[WatchDog_WriteToLidoSampleFile].lowerCompTimeLimit ;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].timeStampLastKick 				= 0;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].sourceIsActive 					  = FALSE;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].requestForDeactivation		= FALSE;
	watchDogKickSources[WatchDog_WriteToLidoSampleFile].sourceForceDisabled				= FALSE;

	watchDogKickSources[WatchDog_TakeLidoSample].isSingleCheckWatchdogSouce				= TRUE;
	watchDogKickSources[WatchDog_TakeLidoSample].lowerCompTimeLimit 					    = 0;
	watchDogKickSources[WatchDog_TakeLidoSample].uppwerCompTimeLimit 					    = 2000;
	watchDogKickSources[WatchDog_TakeLidoSample].measuredCompTime 						    = watchDogKickSources[WatchDog_TakeLidoSample].lowerCompTimeLimit;
	watchDogKickSources[WatchDog_TakeLidoSample].timeStampLastKick 						    = 0;
	watchDogKickSources[WatchDog_TakeLidoSample].sourceIsActive 						      = FALSE;
	watchDogKickSources[WatchDog_TakeLidoSample].requestForDeactivation					  = FALSE;
	watchDogKickSources[WatchDog_TakeLidoSample].sourceForceDisabled					    = FALSE;

	watchDogKickSources[WatchDog_ToggleEnableSampling].isSingleCheckWatchdogSouce	= TRUE;
	watchDogKickSources[WatchDog_ToggleEnableSampling].lowerCompTimeLimit 				= 0;
	watchDogKickSources[WatchDog_ToggleEnableSampling].uppwerCompTimeLimit 				= FILE_OPEN_CLOSE_WRITE_MAX_DURATION_MS;
	watchDogKickSources[WatchDog_ToggleEnableSampling].measuredCompTime 				  = watchDogKickSources[WatchDog_ToggleEnableSampling].lowerCompTimeLimit;
	watchDogKickSources[WatchDog_ToggleEnableSampling].timeStampLastKick 				  = 0;
	watchDogKickSources[WatchDog_ToggleEnableSampling].sourceIsActive 					  = FALSE;
	watchDogKickSources[WatchDog_ToggleEnableSampling].requestForDeactivation			= FALSE;
	watchDogKickSources[WatchDog_ToggleEnableSampling].sourceForceDisabled				= FALSE;

	watchDogKickSources[WatchDog_MeasureTaskRuns].isSingleCheckWatchdogSouce			= FALSE;
	watchDogKickSources[WatchDog_MeasureTaskRuns].lowerCompTimeLimit 					    = 0;
	watchDogKickSources[WatchDog_MeasureTaskRuns].uppwerCompTimeLimit 					  = 3000;
	watchDogKickSources[WatchDog_MeasureTaskRuns].measuredCompTime 					      = watchDogKickSources[WatchDog_MeasureTaskRuns].lowerCompTimeLimit;
	watchDogKickSources[WatchDog_MeasureTaskRuns].kickIntervallXSampleIntervall		= TRUE;
	watchDogKickSources[WatchDog_MeasureTaskRuns].maxKickIntervallLimitRaw				= 3000;
	watchDogKickSources[WatchDog_MeasureTaskRuns].maxKickIntervallLimit  				  = 3000;
	watchDogKickSources[WatchDog_MeasureTaskRuns].timeStampLastKick 					    = 0;
	watchDogKickSources[WatchDog_MeasureTaskRuns].sourceIsActive 						      = FALSE;
	watchDogKickSources[WatchDog_MeasureTaskRuns].requestForDeactivation				  = FALSE;
	watchDogKickSources[WatchDog_MeasureTaskRuns].sourceForceDisabled					    = FALSE;

#if PL_CONFIG_HAS_WATCHDOG
	if (xTaskCreate(WatchDog_Task, "WatchDog", 1500/sizeof(StackType_t), NULL,  configMAX_PRIORITIES-2, &watchDogTaskHandle) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}
#endif
}

//Function also used to activate a source
void WatchDog_StartComputationTime(WatchDog_KickSource_e kickSource) {
	taskENTER_CRITICAL();
	if(!watchDogKickSources[kickSource].sourceForceDisabled) {
		watchDogKickSources[kickSource].timeStampLastKick = xTaskGetTickCount();
		watchDogKickSources[kickSource].sourceIsActive 					= TRUE;
		watchDogKickSources[kickSource].requestForDeactivation 			= FALSE;
		watchDogKickSources[kickSource].compTimeMeasurementRunning		= TRUE;
	}
	taskEXIT_CRITICAL();
}

void WatchDog_StopComputationTime(WatchDog_KickSource_e kickSource) {
  taskENTER_CRITICAL();
	watchDogKickSources[kickSource].measuredCompTime = xTaskGetTickCount() - watchDogKickSources[kickSource].timeStampLastKick;
	watchDogKickSources[kickSource].compTimeMeasurementRunning		= FALSE;

	if(!watchDogKickSources[kickSource].isSingleCheckWatchdogSouce)	{
		if(watchDogKickSources[kickSource].kickIntervallXSampleIntervall)	{
			uint8_t sampleIntervall_s;
			AppDataFile_GetSampleIntervall(&sampleIntervall_s);
			watchDogKickSources[kickSource].maxKickIntervallLimit = watchDogKickSources[kickSource].maxKickIntervallLimitRaw * sampleIntervall_s;
		}	else {
			watchDogKickSources[kickSource].maxKickIntervallLimit;
		}
	}	else {
		watchDogKickSources[kickSource].requestForDeactivation 	= TRUE;
	}
	taskEXIT_CRITICAL();
}

void WatchDog_DisableSource(WatchDog_KickSource_e kickSource) {
  taskENTER_CRITICAL();
	watchDogKickSources[kickSource].sourceForceDisabled 		= TRUE;
	watchDogKickSources[kickSource].sourceIsActive 					= FALSE;
	taskEXIT_CRITICAL();
}

void WatchDog_EnableSource(WatchDog_KickSource_e kickSource) {
  taskENTER_CRITICAL();
	watchDogKickSources[kickSource].sourceForceDisabled 		= FALSE;
	watchDogKickSources[kickSource].sourceIsActive 					= TRUE;
	taskEXIT_CRITICAL();
	WatchDog_StartComputationTime(kickSource);
}
