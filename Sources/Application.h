/*
 * Application.h
 *
 *  Created on: 17.02.2018
 *      Author: Erich Styger
 */

#ifndef SOURCES_APPLICATION_H_
#define SOURCES_APPLICATION_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void APP_Run(void);

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
	bool	 userButtonMarker;
} liDoSample_t;



#endif /* SOURCES_APPLICATION_H_ */
