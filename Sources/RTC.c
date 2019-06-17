/*
 * RTC.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */
#include "RTC.h"
#include "RTC1.h"
#include "TmDt1.h"

static LDD_TDeviceData* rtcDeviceDataHandle;

void RTC_getTimeUnixFormat(int32_t *rtcTimeUnix)
{
	TIMEREC time;
	DATEREC date;

	TmDt1_GetInternalRTCTimeDate(&time,&date);
	*rtcTimeUnix = TmDt1_TimeDateToUnixSeconds(&time, &date, 0);
}

void RTC_setTimeUnixFormat(int32_t rtcTimeUnix)
{
	TIMEREC time;
	DATEREC date;

	TmDt1_UnixSecondsToTimeDate(rtcTimeUnix, 0, &time, &date);
	TmDt1_SetInternalRTCTimeDate(&time,&date);
}

void RTC_init(bool softInit)
{
	rtcDeviceDataHandle = RTC1_Init(NULL, softInit);
}
