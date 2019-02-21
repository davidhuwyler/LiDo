/*
 * LightSensor.h
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 */

#ifndef SOURCES_LIGHTSENSOR_H_
#define SOURCES_LIGHTSENSOR_H_

#include <stdint.h>

typedef struct {
	uint16_t xChannelValue;
	uint16_t yChannelValue;
	uint16_t zChannelValue;
	uint16_t nChannelValue;
} LightChannels_t;

void LightSensor_init(void);
uint8_t LightSensor_autoZeroBlocking(void);
uint8_t LightSensor_getChannelValuesBlocking(LightChannels_t* channels);



#endif /* SOURCES_LIGHTSENSOR_H_ */
