/*
 * WatchDog.c
 *
 *  Created on: Mar 16, 2019
 *      Author: dave
 */
#include "Platform.h"
#include "WatchDog.h"
#include "WDog1.h"
#include "KIN1.h"
#include "FRTOS1.h"
#include "CS1.h"

static const uint16 watchDogKickIntervallPerSource[WatchDog_NOF_KickSources] = {2000};

static uint16 watchDogTimeLeftUntilDogBitesPerSource[WatchDog_NOF_KickSources];


//TODO Implement "Magic Code" from http://www.ganssle.com/watchdogs.htm

static void WatchDog_Task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  static bool feedTheDog = TRUE;

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();

		for(int i = 0; i< WatchDog_NOF_KickSources ; i++)
		{
			CS1_CriticalVariable();
			CS1_EnterCritical();
			if(watchDogTimeLeftUntilDogBitesPerSource[i] == 0)
			{
				feedTheDog = FALSE; // --> WatchDog Source ran out... Reset!
				CS1_ExitCritical();
				break;
			}
			else if(watchDogTimeLeftUntilDogBitesPerSource[i]>=WATCHDOG_TASK_DELAY)
			{
				watchDogTimeLeftUntilDogBitesPerSource[i] -= WATCHDOG_TASK_DELAY;
			}
			else
			{
				watchDogTimeLeftUntilDogBitesPerSource[i] = 0;
			}
			CS1_ExitCritical();
		}

		if(feedTheDog)
		{
			WDog1_Clear();
		}
		else
		{
			//KIN1_SoftwareReset();
			for(;;);
		}

	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(WATCHDOG_TASK_DELAY));
  } /* for */
}

void WatchDog_Init(void)
{
	//initial Dog feed...
	for(int i = 0; i< WatchDog_NOF_KickSources ; i++)
	{
		watchDogTimeLeftUntilDogBitesPerSource[i] = watchDogKickIntervallPerSource[i];
	}

	if (xTaskCreate(WatchDog_Task, "WatchDog", 1000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}
}

void WatchDog_Kick(WatchDog_KickSource_e kickSource)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	watchDogTimeLeftUntilDogBitesPerSource[kickSource] = watchDogKickIntervallPerSource[kickSource];
	CS1_ExitCritical();
}

