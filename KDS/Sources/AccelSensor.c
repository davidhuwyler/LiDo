/*
 * AccelSens.c
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 *
 *
 *  Driver for the ST LIS2DH Accelerometer via I2C
 */
#include "Platform.h"
#if PL_CONFIG_HAS_ACCEL_SENSOR
#include "Application.h"
#include "AccelSensor.h"
#include "GI2C1.h"
#include "LED_R.h"
#include "WAIT1.h"
#if PL_CONFIG_HAS_SENSOR_PWR_PIN
  #include "PIN_SENSOR_PWR.h"
#endif
#if PL_CONFIG_HAS_ACCEL_ISR1_PIN
  #include "ACC_INT1.h"
#endif

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

#define ACCEL_SENS_I2C_TEMPERATURE_OFFSET  (11) /* NOTE: the temperature sensor is not absulute, so there needs to be a offset calibration for each device! */

void AccelSensor_init(void) {
	uint8_t i2cData;
	uint8_t res;

#if PL_CONFIG_HAS_SENSOR_PWR_PIN
	//PowerSensors
	PIN_SENSOR_PWR_ClrVal(); //LowActive!
#endif
	WAIT1_Waitms(1);
	res = GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG1 , 0x1F);		//0x1F = 1Hz samples & LowpowerMode on; 0x0F = PowerDownMode
	res |= GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_TEMP_CFG , 0xC0);		//Enable Temp. Measurement
	if(res != ERR_OK) {
		APP_FatalError();
	}
}

uint8_t AccelSensor_getValues(AccelAxis_t* values) {
	uint8_t res = 0, temp;
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTX_H , &values->xValue );
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTY_H , &values->yValue );
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUTZ_H , &values->zValue );

	res |= GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG4 , 0x80);		//BDU = 1 (needed for Temp.Sensing)
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUT_TEMP_L , &temp ); //Not needed for data acquisition but has to be read out...
	res |= GI2C1_ReadByteAddress8(ACCEL_SENS_I2C_ADDRESS,ACCEL_SENS_I2C_REGISTER_OUT_TEMP_H , &temp );
	res |= GI2C1_WriteByteAddress8(ACCEL_SENS_I2C_ADDRESS, ACCEL_SENS_I2C_REGISTER_CTRL_REG4 , 0x00);		//BDU = 1 (needed for Temp.Sensing)
	values->temp = temp + ACCEL_SENS_I2C_TEMPERATURE_OFFSET; //Tempoffset needed...
	return res;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  AccelAxis_t values;
  uint8_t buf[32];

  CLS1_SendStatusStr((unsigned char*)"LIS2DH", (const unsigned char*)"\r\n", io->stdOut);
  uint8_t res = AccelSensor_getValues(&values);

  if (res==ERR_OK) {
    UTIL1_strcpy(buf, sizeof(buf), (unsigned char*)"x: ");
    UTIL1_strcatNum8s(buf, sizeof(buf), values.xValue);
    UTIL1_strcat(buf, sizeof(buf), (unsigned char*)", y: ");
    UTIL1_strcatNum8s(buf, sizeof(buf), values.yValue);
    UTIL1_strcat(buf, sizeof(buf), (unsigned char*)", z: ");
    UTIL1_strcatNum8s(buf, sizeof(buf), values.zValue);
    UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  } else {
    UTIL1_strcpy(buf, sizeof(buf), (unsigned char*)"ERROR\r\n");
  }
  CLS1_SendStatusStr((unsigned char*)"  Accel", buf, io->stdOut);

  if (res==ERR_OK) {
    UTIL1_Num8sToStr(buf, sizeof(buf), values.temp);
    UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" Degree Celsius\r\n");
  } else {
    UTIL1_strcpy(buf, sizeof(buf), (unsigned char*)"ERROR\r\n");
  }
  CLS1_SendStatusStr((unsigned char*)"  Temp", buf, io->stdOut);

  UTIL1_Num8sToStr(buf, sizeof(buf), ACCEL_SENS_I2C_TEMPERATURE_OFFSET);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  CLS1_SendStatusStr((unsigned char*)"  Temp offset", buf, io->stdOut);
#if PL_CONFIG_HAS_ACCEL_ISR1_PIN
  UTIL1_strcpy(buf, sizeof(buf), ACC_INT1_GetVal()?(unsigned char*)"HIGH\r\n":(unsigned char*)"LOW\r\n");
  CLS1_SendStatusStr((unsigned char*)"  INT1", buf, io->stdOut);
#endif
  return ERR_OK;
}

uint8_t AccelSensor_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "LIS2DH help")==0)  {
    CLS1_SendHelpStr((unsigned char*)"LIS2DH", (const unsigned char*)"Group of LIS2DH commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  } else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LIS2DH status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  return ERR_OK;
}
#endif /* PL_CONFIG_HAS_ACCEL_SENSOR */
