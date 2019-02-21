/*
 * AccelSens.c
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 */
#include "AccelSensor.h"
#include "GI2C1.h"
#include "LED1.h"
#include "WAIT1.h"

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

#define ACCEL_SENS_I2C_REGISTER_TEMP_CFG 0x1F
#define ACCEL_SENS_I2C_REGISTER_OUT_TEMP_H 0x0D
#define ACCEL_SENS_I2C_REGISTER_OUT_TEMP_L 0x0C

void AccelSensor_init(void)
{
	uint8_t i2cData;
	uint8_t res;

	res = GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG1 , 0x1F);		//0x1F = 1Hz samples & LowpowerMode on; 0x0F = PowerDownMode
	res |= GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_TEMP_CFG , 0xC0);		//Enable Temp. Measurement

	if(res != ERR_OK)
	{
		for(;;)//IIC Error
		{
			LED1_Neg();
			WAIT1_Waitms(50);
		}
	}
}

uint8_t AccelSensor_getValues(AccelAxis_t* values)
{
	uint8_t res = 0, temp;
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTX_H , &values->xValue );
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTY_H , &values->yValue );
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTZ_H , &values->zValue );

	res |= GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG4 , 0x80);		//BDU = 1 (needed for Temp.Sensing)
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUT_TEMP_L , &temp ); //Not needed for data acquisition but has to be read out...
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUT_TEMP_H , &temp );
	res |= GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG4 , 0x00);		//BDU = 1 (needed for Temp.Sensing)
	values->temp = temp + 16; //Tempoffset needed...
	return res;
}


