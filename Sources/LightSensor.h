/*
 * LightSensor.h
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 *
 *  Driver for the AS7264N LightSensor via I2C
 */

#ifndef SOURCES_LIGHTSENSOR_H_
#define SOURCES_LIGHTSENSOR_H_

#include <stdint.h>
#include "CLS1.h"

typedef struct {
	uint16_t xChannelValue;
	uint16_t yChannelValue;
	uint16_t zChannelValue;
	uint16_t nChannelValue;
} LightChannels_t;

typedef enum {
	LightSensor_Bank0_X_Y_B_B = 0,
	LightSensor_Bank1_X_Y_Z_IR = 1
} LightSensorBank;

void LightSensor_init(void);
uint8_t LightSensor_autoZeroBlocking(void);
uint8_t LightSensor_getChannelValues(LightChannels_t* bank0,LightChannels_t* bank1);
uint8_t LightSensor_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

#endif /* SOURCES_LIGHTSENSOR_H_ */
