/*
 * AccelSens.h
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 *
 *  Driver for the ST LIS2DH Accelerometer via I2C
 */

#ifndef SOURCES_ACCELSENS_H_
#define SOURCES_ACCELSENS_H_

#include "Platform.h"
#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
  uint8_t AccelSensor_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
#endif

typedef enum { /* bits corresponding to register position */
  LIS2DH_CTRL_REG1_POWERMODE_POWERDOWN  = (0b0000<<4),
  LIS2DH_CTRL_REG1_POWERMODE_1HZ        = (0b0001<<4),
  LIS2DH_CTRL_REG1_POWERMODE_10HZ       = (0b0010<<4),
  LIS2DH_CTRL_REG1_POWERMODE_25HZ       = (0b0011<<4),
  LIS2DH_CTRL_REG1_POWERMODE_50HZ       = (0b0100<<4),
  LIS2DH_CTRL_REG1_POWERMODE_100HZ      = (0b0101<<4),
  LIS2DH_CTRL_REG1_POWERMODE_200HZ      = (0b0110<<4),
  LIS2DH_CTRL_REG1_POWERMODE_400HZ      = (0b0111<<4),
  LIS2DH_CTRL_REG1_POWERMODE_1620HZ     = (0b1000<<4),
  LIS2DH_CTRL_REG1_POWERMODE_NORMAL     = (0b1001<<4),
  LIS2DH_CTRL_REG1_POWERMODE_MASK_BITS  = (0b1111<<4),
} LIS2DH_CTRL_REG1_PowerMode;

uint8_t AccelSensor_SetPowerMode(LIS2DH_CTRL_REG1_PowerMode mode);

uint8_t AccelSensor_SetCtrlReg1(uint8_t reg1);

typedef struct {
	uint8_t xValue;  // 1digit = 16mG (8Bit signed)
	uint8_t yValue;
	uint8_t zValue;
	int8_t temp;	 //Temperature in °C (1°C Resolution) (8Bit signed)
} AccelAxis_t;

uint8_t AccelSensor_GetTemperatureSensorEnabled(bool *enabled);

uint8_t AccelSensor_EnableTemperatureSensor(void);

uint8_t AccelSensor_DisableTemperatureSensor(void);

uint8_t AccelSensor_getValues(AccelAxis_t* values);

uint8_t AccelSensor_WhoAmI(uint8_t *id);

void AccelSensor_Init(void);

#endif /* SOURCES_ACCELSENS_H_ */
