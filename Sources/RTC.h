/*
 * RTC.h
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */

#ifndef SOURCES_RTC_H_
#define SOURCES_RTC_H_

void RTC_init(bool softInit);
uint8_t RTC_setTime(LDD_RTC_TTime* rtcTime);
void RTC_getTime(LDD_RTC_TTime* rtcTime);
void RTC_getTimeUnixFormat(uint32_t* rtcTimeUnix);
uint8_t RTC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);



#endif /* SOURCES_RTC_H_ */
