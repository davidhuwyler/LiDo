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
#include "CS1.h"
#include "AppDataFile.h"
#include "SDEP.h"
#include "WDog1.h"
#include "LowPower.h"

// Configure Watchdog Kicksources:
static const uint16 watchDogKickIntervallPerSource[WatchDog_NOF_KickSources][3] =
{
		{		//WatchDog_KickedByApplication_c
				1100,							//KickIntervall [ms] x SampleIntervall [s] (Ex: Sampleintervall = 2s Kickintervall = 1100ms: Dog needs to be fed every 2200ms
				0,								//LowerBoundary ComputationTime im Ms for the Main Task
				960,							//HighBoundary ComputationTime im Ms for the Main Task
		}
};

static uint16 watchDogKickSourceParams[WatchDog_NOF_KickSources][2]; //{ IntervallUntilDogBytes, LastRegisteredComputationTime }


static void WatchDog_Task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  static bool feedTheDog = TRUE;
  int i;

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();

		for(i = 0; i< WatchDog_NOF_KickSources ; i++)
		{
			CS1_CriticalVariable();
			CS1_EnterCritical();
			if(watchDogKickSourceParams[i][0] == 0)		//FeedDog intervall ran out?
			{
				feedTheDog = FALSE; // --> WatchDog Source ran out... Reset!
				CS1_ExitCritical();
				break;
			}
			else if(watchDogKickSourceParams[i][1] < watchDogKickIntervallPerSource[i][1] || //ComputationTime of the Source in boundary?
					watchDogKickSourceParams[i][1] > watchDogKickIntervallPerSource[i][2] )
			{
				feedTheDog = FALSE; // --> WatchDog CompTime not in boundary... Reset!
				CS1_ExitCritical();
				break;
			}
			else if(watchDogKickSourceParams[i][0]>=WATCHDOG_TASK_DELAY)
			{
				watchDogKickSourceParams[i][0] -= WATCHDOG_TASK_DELAY;
			}
			else
			{
				watchDogKickSourceParams[i][0] = 0;
			}
			CS1_ExitCritical();
		}


		if(!feedTheDog)//Reset!
		{
			//TODO Power Off SPIF und Sensoren
			WDog1_Clear();
			//Log Watchdog Reset:
			switch(i)
			{
				case 0 : //Kicksource: WatchDog_KickedByApplication_c
					SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_WATCHDOG_RESET,"WatchDogReset Source: MainTask");
					break;
				default:
					SDEP_InitiateNewAlert(SDEP_ALERT_WATCHDOG_RESET);
					break;
			}

			vTaskDelay(pdMS_TO_TICKS(100)); //Let the Shell print out the Alert message...
			//KIN1_SoftwareReset();
			for(;;);
		}
		else
		{
			WDog1_Clear();
		}

		if(LowPower_StopModeIsEnabled())
		{
			uint8_t sampleIntervall_s;
			AppDataFile_GetSampleIntervall(&sampleIntervall_s);
			vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(sampleIntervall_s*WATCHDOG_TASK_DELAY));
		}
		else
		{
			vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(WATCHDOG_TASK_DELAY));
		}
  } /* for */
}

void WatchDog_Init(void)
{
	//initial Dog feed...
	for(int i = 0; i< WatchDog_NOF_KickSources ; i++)
	{
		watchDogKickSourceParams[i][0] = watchDogKickIntervallPerSource[i][0];
		watchDogKickSourceParams[i][1] = watchDogKickIntervallPerSource[i][1];
	}

	if (xTaskCreate(WatchDog_Task, "WatchDog", 1000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}
}

void WatchDog_Kick(WatchDog_KickSource_e kickSource, uint16_t computationDuration_ms)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	uint8_t sampleIntervall_s;
	AppDataFile_GetSampleIntervall(&sampleIntervall_s);
	watchDogKickSourceParams[kickSource][0] = watchDogKickIntervallPerSource[kickSource][0] * sampleIntervall_s;
	watchDogKickSourceParams[kickSource][1] = computationDuration_ms;
	CS1_ExitCritical();
}
