/*
 * LightSensor.c
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 *
 *  Driver for the AS7264N LightSensor via I2C
 */
#include "LightSensor.h"
#include "LightSensResetPin.h"
#include "LightSenseInterruptPin.h"
#include "LED1.h"
#include "FRTOS1.h"
#include "GI2C1.h"
#include "WAIT1.h"
#include "CLS1.h"

#define LIGHTSENSOR_I2C_ADDRESS 0x49
#define LIGHTSENSOR_I2C_REGISTER_DEV_ID 0x10
#define LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_1 0x70
#define LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_2 0x71
#define LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_3 0xB0
#define LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_4 0x88
#define LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_5 0x9A
#define LIGHTSENSOR_I2C_REGISTER_PWR_MODE 0x73
#define LIGHTSENSOR_I2C_REGISTER_INTR_PIN_CONFIG 0x22
#define LIGHTSENSOR_I2C_REGISTER_INTR_POLL_CLR 0xF8
#define LIGHTSENSOR_I2C_REGISTER_INTR_POLL_EN 0xF9
#define LIGHTSENSOR_I2C_REGISTER_CONFIG_INT_T 0xD9		// Integrationtime (whole Byte)
#define LIGHTSENSOR_I2C_REGISTER_CONFIG_GAIN_IDRV 0xB9	// GAIN: Bit0 + Bit0    |     IDRV: Bit6 + Bit7 (IDRV is the LED Drive Current. Not used)
#define LIGHTSENSOR_I2C_REGISTER_CONFIG_BANK 0xDB		// Photodiode Bank to read (BIT7 1 or 0)
#define LIGHTSENSOR_I2C_REGISTER_CONFIG_WTIME 0xDA		// Waittime (whole Byte)
#define LIGHTSENSOR_I2C_REGISTER_CONFIG_DATA_EN 0xFA
#define LIGHTSENSOR_I2C_REGISTER_AUTO_ZERO 0xBA
#define LIGHTSENSOR_I2C_REGISTER_TMP_H 0xD1			 	//Temperature High Byte
#define LIGHTSENSOR_I2C_REGISTER_TMP_L 0xD2				//Temperature Low Byte
#define LIGHTSENSOR_I2C_REGISTER_TMP_CONFIG 0xD3
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_N_DATA_L 0xDC
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_N_DATA_H 0xDD
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_Y_DATA_L 0xDE
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_Y_DATA_H 0xDF
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_Z_DATA_L 0xEC
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_Z_DATA_H 0xED
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_X_DATA_L 0xEE
#define LIGHTSENSOR_I2C_REGISTER_CHANNEL_X_DATA_H 0xEF

void LightSensor_init(void)
{
	//Reset Sensor
	LightSensResetPin_ClrVal();
	//WAIT1_Waitms(50);
	LightSensResetPin_SetVal();
	//WAIT1_Waitms(1);


//	if(GI2C1_ReadAddress(LIGHTSENSOR_I2C_ADDRESS, &i2cRegister, sizeof(i2cRegister), &i2cData, sizeof(i2cData)) != pdPASS)
//	{
//		for(;;); //IIC Error
//	}

	uint8_t i2cData;
	uint8_t res;

	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_1 , 0x8A);		//Register must be initialized with 0x8A (Datasheet p. 20)
	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_2 , 0x02);		//Register must be initialized with 0x02 (Datasheet p. 20)
	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_3 , 0x02);		//Register must be initialized with 0x02 (Datasheet p. 20)
	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_4 , 0x00);		//Register must be initialized with 0x00 (Datasheet p. 20)
	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_DEV_CONFIG_5 , 0x00);		//Register must be initialized with 0x00 (Datasheet p. 20)

	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_PIN_CONFIG , 0xCA);		//Enable Interrupt after Conversion finished
	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_POLL_CLR , 0x00);		//Clear Interrupt
	res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_POLL_EN , 0x04);		//Enable ChannelData for Interrupts

	if(res != ERR_OK)
	{
		for(;;)//IIC Error
		{
			LED1_Neg();
			WAIT1_Waitms(50);
		}
	}
}


uint8_t LightSensor_autoZeroBlocking(void)
{
	uint8_t i2cData;
	uint8_t res = GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_AUTO_ZERO , 0x0F);			//Autozero all Channels
	res = GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_AUTO_ZERO , &i2cData );
	while(i2cData & 0x80 != 1) //Wait for autoZero to complete
	{
		GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_AUTO_ZERO , &i2cData );
	}
	return res;
}


uint8_t LightSensor_getChannelValuesBlocking(LightChannels_t* channels, LightSensorBank bankToSample)
{
	uint8_t i2cData;
	uint8_t res;
	uint8_t bankToSampleByteValue = (((uint8_t)bankToSample)*0x80);

	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_PWR_MODE , 0x00);			//Disable Lowpower Mode
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_BANK , bankToSampleByteValue);			//0x80 = Bank1 (x,y,z,NIR) 0x00 = Bank0 (x,y,b,b)
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_GAIN_IDRV , 0x03);	//Gain = b00=1x; b01=3.7x; b10=16x; b11=64x;
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_INT_T , 0xF0);		//IntegrationTime: (256 - value) * 2.8ms
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_WTIME , 0xF0);		//Time between Conversions: (256 - value) * 2.8ms

	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_DATA_EN , 0x01 ); //Enable Conversion
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_POLL_CLR , 0x04 );	//Clear Interrupt
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_DATA_EN , 0x03 ); //Start Conversion

	while(!LightSenseInterruptPin_GetVal()){}

	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_POLL_CLR , &i2cData );
	while(i2cData != 0x04)
	{
		GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_POLL_CLR , &i2cData );
	}

	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_DATA_EN , 0x83 ); //LatchData Conversion

	uint8_t n_L, n_H, x_L, x_H, y_L, y_H, z_L, z_H;
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_N_DATA_H , &n_H );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_N_DATA_L , &n_L );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_X_DATA_H , &x_H );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_X_DATA_L , &x_L );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_Y_DATA_H , &y_H );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_Y_DATA_L , &y_L );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_Z_DATA_H , &z_H );
	res |= GI2C1_ReadByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CHANNEL_Z_DATA_L , &z_L );

	channels->nChannelValue = ((uint16_t)n_L | ((uint16_t)n_H<<8));
	channels->xChannelValue = ((uint16_t)x_L | ((uint16_t)x_H<<8));
	channels->yChannelValue = ((uint16_t)n_L | ((uint16_t)y_H<<8));
	channels->zChannelValue = ((uint16_t)n_L | ((uint16_t)z_H<<8));

	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_INTR_POLL_CLR , 0x00);		//Clear Interrupt
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_DATA_EN , 0x00 );    //Disable Conversion
	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_PWR_MODE , 0x02);			//Enable Lowpower Mode

	return res;
}

static uint8_t PrintBank0(CLS1_ConstStdIOType *io)
{
	LightChannels_t lightChannels;
	uint8_t res = LightSensor_getChannelValuesBlocking(&lightChannels,LightSensor_Bank0_X_Y_B_B);
	CLS1_SendStr("X value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.xChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("Y value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.yChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("B440 value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.zChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("B480 value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.nChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	return res;
}

static uint8_t PrintBank1(CLS1_ConstStdIOType *io)
{
	LightChannels_t lightChannels;
	uint8_t res = LightSensor_getChannelValuesBlocking(&lightChannels,LightSensor_Bank1_X_Y_Z_IR);
	CLS1_SendStr("X value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.xChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("Y value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.yChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("Z value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.zChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	CLS1_SendStr("IR value: ",io->stdOut);
	CLS1_SendNum16u(lightChannels.nChannelValue,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
	return res;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"LightSens", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t LightSensor_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io)
{
  uint8_t res = ERR_OK;
  const uint8_t *p;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "LightSens help")==0)
  {
    CLS1_SendHelpStr((unsigned char*)"LightSens", (const unsigned char*)"Group of LightSensor (AS7264N) commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  getBank0", (const unsigned char*)"Measures SensorBank0(x,y,b,b)\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  getBank1", (const unsigned char*)"Measures SensorBank1(x,y,z,IR)C\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  }
  else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LightSens status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  else if (UTIL1_strcmp((char*)cmd, "LightSens getBank0")==0)
  {
    *handled = TRUE;
    return PrintBank0(io);
  }
  else if (UTIL1_strcmp((char*)cmd, "LightSens getBank1")==0) {

	*handled = TRUE;
	return PrintBank1(io);
  }
  return res;
}





