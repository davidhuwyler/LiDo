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

void AccelSensor_init(void)
{
	uint8_t i2cData;
	uint8_t res;

	res = GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG1 , 0x1F);		//0x1F = 1Hz samples & LowpowerMode on; 0x0F = PowerDownMode
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
	uint8_t res = 0;
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTX_H , &values->xValue );
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTY_H , &values->yValue );
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTZ_H , &values->zValue );
	return res;
}


