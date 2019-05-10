/*
 * WatchDog.h
 *
 *  Created on: Mar 16, 2019
 *      Author: dave
 */

#ifndef SOURCES_WATCHDOG_H_
#define SOURCES_WATCHDOG_H_

#define WATCHDOG_TASK_DELAY 500 //Needs to be smaller than the Watchdogperiode

typedef enum
{
	WatchDog_OpenCloseLidoSampleFile,
	WatchDog_WriteToLidoSampleFile,
	WatchDog_TakeLidoSample,
	WatchDog_ToggleEnableSampling,
	WatchDog_MeasureTaskRunns,
	WatchDog_LiDoInit,
	WatchDog_NOF_KickSources
}WatchDog_KickSource_e;

void WatchDog_Init(void);

void WatchDog_StartComputationTime(WatchDog_KickSource_e kickSource);
void WatchDog_StopComputationTime(WatchDog_KickSource_e kickSource);

void WatchDog_DisableSource(WatchDog_KickSource_e kickSource);
void WatchDog_EnableSource(WatchDog_KickSource_e kickSource);

#endif /* SOURCES_WATCHDOG_H_ */
