/*
 * Application.h
 *
 *  Created on: 17.02.2018
 *      Author: Erich Styger
 */

#ifndef SOURCES_APPLICATION_H_
#define SOURCES_APPLICATION_H_

#include "Platform.h"
#include "CLS1.h"

#define CONFIG_ENABLE_STOPMODE


#define LIDO_SAMPLE_SIZE 22
typedef struct
{
	int32_t  unixTimeStamp;
	uint16_t lightChannelX;
	uint16_t lightChannelY;
	uint16_t lightChannelZ;
	uint16_t lightChannelIR;
	uint16_t lightChannelB440;
	uint16_t lightChannelB490;
	int8_t   accelX;
	int8_t   accelY;
	int8_t   accelZ;
	uint8_t	 temp;
	uint8_t	 crc;
} liDoSample_t;


void APP_Run(void);
uint8_t APP_getCurrentSample(liDoSample_t* sample, int32 unixTimestamp);
void APP_setMarkerInLog(void);
void APP_toggleEnableSampling(void);
void APP_requestForSoftwareReset(void);
uint8_t APP_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

#endif /* SOURCES_APPLICATION_H_ */
