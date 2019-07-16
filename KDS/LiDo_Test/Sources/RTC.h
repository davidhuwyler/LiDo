/*
 * RTC.h
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */

#ifndef SOURCES_RTC_H_
#define SOURCES_RTC_H_

#include "Platform.h"

void RTC_getTimeUnixFormat(int32_t *rtcTimeUnix);
void RTC_setTimeUnixFormat(int32_t rtcTimeUnix);
void RTC_EnableRTCInterrupt(void);
void RTC_DisableRTCInterrupt(void);
void RTC_init(bool softInit);

#endif /* SOURCES_RTC_H_ */
