/*
 * AccelSens.h
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 */

#include <stdint.h>

#ifndef SOURCES_ACCELSENS_H_
#define SOURCES_ACCELSENS_H_




typedef struct {
	uint8_t xValue;  // 1digit = 16mG (8Bit signed)
	uint8_t yValue;
	uint8_t zValue;
	uint8_t temp;	 //Temperature in °C (1°C Resolution) (8Bit signed)
} AccelAxis_t;

void AccelSensor_init(void);
uint8_t AccelSensor_getValues(AccelAxis_t* values);


#endif /* SOURCES_ACCELSENS_H_ */
