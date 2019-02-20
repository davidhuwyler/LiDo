/*
 * AccelSens.h
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 */

#include <stdint.h>

#ifndef SOURCES_ACCELSENS_H_
#define SOURCES_ACCELSENS_H_


#define ACCEL_SENS_I2C_ADDRESS 0x19

#define ACCEL_SENS_I2C_REGISTER_CTRL_REG1 0x20 //SamplingFrequenz und LowpowerMode (Datasheet p33)
#define ACCEL_SENS_I2C_REGISTER_CTRL_REG2 0x21 //LowPass filter.. (Datasheet p34)
#define ACCEL_SENS_I2C_REGISTER_CTRL_REG3 0x22 //Interrupts (Datasheet p34)
#define ACCEL_SENS_I2C_REGISTER_CTRL_REG4 0x23 //HighResMode, Selftest .. (Datasheet p35)
#define ACCEL_SENS_I2C_REGISTER_CTRL_REG5 0x24
#define ACCEL_SENS_I2C_REGISTER_CTRL_REG6 0x25

#define ACCEL_SENS_I2C_REGISTER_REFERENCE 0x26
#define ACCEL_SENS_I2C_REGISTER_OUTX_L 0x28
#define ACCEL_SENS_I2C_REGISTER_OUTX_H 0x29
#define ACCEL_SENS_I2C_REGISTER_OUTY_L 0x2A
#define ACCEL_SENS_I2C_REGISTER_OUTY_H 0x2B
#define ACCEL_SENS_I2C_REGISTER_OUTZ_L 0x2C
#define ACCEL_SENS_I2C_REGISTER_OUTZ_H 0x2D

typedef struct {
	uint8_t xValue;  // 1digit = 16mG
	uint8_t yValue;
	uint8_t zValue;
} AccelAxis_t;

void AccelSensor_init(void);
uint8_t AccelSensor_getValues(AccelAxis_t* values);

#endif /* SOURCES_ACCELSENS_H_ */
