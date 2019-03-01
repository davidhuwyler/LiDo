/*
 * AccelSens.c
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 *
 *
 *  Driver for the ST LIS2DH Accelerometer via I2C
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

static uint8_t PrintValues(CLS1_ConstStdIOType *io)
{
	AccelAxis_t values;
	uint8_t res = AccelSensor_getValues(&values);
	CLS1_SendStr("X accel: ",io->stdOut);
	CLS1_SendNum8s(values.xValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("Y accel: ",io->stdOut);
	CLS1_SendNum8s(values.yValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("Z accel: ",io->stdOut);
	CLS1_SendNum8s(values.zValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("Temperature: ",io->stdOut);
	CLS1_SendNum8u(values.temp,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	return res;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"AccelSens", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t AccelSensor_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io)
{
  uint8_t res = ERR_OK;
  const uint8_t *p;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "AccelSens help")==0)
  {
    CLS1_SendHelpStr((unsigned char*)"AccelSens", (const unsigned char*)"Group of Accelerometer (LIS2DH) + Temp. commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  getValues", (const unsigned char*)"Measures SensorBank0(x,y,b,b)\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  }
  else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LightSens status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  else if (UTIL1_strcmp((char*)cmd, "AccelSens getValues")==0)
  {
    *handled = TRUE;
    return PrintValues(io);
  }
  return res;
}


