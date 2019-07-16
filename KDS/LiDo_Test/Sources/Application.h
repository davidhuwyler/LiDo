/*
 * Application.h
 *
 *  Created on: 17.02.2018
 *      Author: Erich Styger
 */

#ifndef SOURCES_APPLICATION_H_
#define SOURCES_APPLICATION_H_

#include "Platform.h"

#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
  uint8_t APP_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
#endif

#define LIDO_FILE_HEADER "LiDo Sample File V1.0"

#define LIDO_SAMPLE_SIZE 23
typedef struct
{
	int32_t  unixTimeStamp;
	uint8_t	 lightIntTime;
	uint8_t	 lightGain;
	uint16_t lightChannelX;
	uint16_t lightChannelY;
	uint16_t lightChannelZ;
	uint16_t lightChannelIR;
	uint16_t lightChannelB440;
	uint16_t lightChannelB490;
	int8_t   accelX;
	int8_t   accelY;
	int8_t   accelZ;
	uint8_t	 temp; //Bit7 = UserButtonMarker
	uint8_t	 crc;
} liDoSample_t;

void APP_Run(void);
void APP_CloseSampleFile(void);
uint8_t APP_getCurrentSample(liDoSample_t* sample, int32 unixTimestamp,bool forceSample);
void APP_setMarkerInLog(void);
void APP_toggleEnableSampling(void);
void APP_requestForSoftwareReset(void);
void APP_requestForPowerOff(void);
void APP_suspendSampleTask(void);
void APP_resumeSampleTaskFromISR(void);

void APP_FatalError(const char *fileName, unsigned int lineNo);

#endif /* SOURCES_APPLICATION_H_ */
