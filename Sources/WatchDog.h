/*
 * WatchDog.h
 *
 *  Created on: Mar 16, 2019
 *      Author: dave
 */

#ifndef SOURCES_WATCHDOG_H_
#define SOURCES_WATCHDOG_H_

//#include <stdint.h>
//#include <stddef.h>
//#include <stdbool.h>

#define WATCHDOG_TASK_DELAY 1000

typedef enum
{
	WatchDog_KickedByApplication_c,
	WatchDog_NOF_KickSources
}WatchDog_KickSource_e;

void WatchDog_Init(void);

void WatchDog_Kick(WatchDog_KickSource_e kickSource);

#endif /* SOURCES_WATCHDOG_H_ */
