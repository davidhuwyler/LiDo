/*
 * LightSensor.c
 *
 *  Created on: Feb 20, 2019
 *      Author: dave
 */
#include "LightSensor.h"
#include "LightSensResetPin.h"
#include "LightSenseInterruptPin.h"
#include "LED1.h"
#include "FRTOS1.h"
#include "GI2C1.h"
#include "WAIT1.h"

static void LightSensor_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  LightChannels_t channels;

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();

	  LED1_Neg();
	  LightSensor_getChannelValuesBlocking(&channels);

	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(1000));
  } /* for */
}

void LightSensor_init(void)
{
	//Init Task
	if (xTaskCreate(LightSensor_task, "LightSensor", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	{
	    for(;;){} /* error! probably out of memory */
	}

	//Reset Sensor
	LightSensResetPin_ClrVal();
	WAIT1_Waitms(50);
	LightSensResetPin_SetVal();
	WAIT1_Waitms(500);


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
		for(;;);//IIC Error
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


uint8_t LightSensor_getChannelValuesBlocking(LightChannels_t* channels)
{
	uint8_t i2cData;
	uint8_t res;

	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_PWR_MODE , 0x00);			//Disable Lowpower Mode

	res |= GI2C1_WriteByteAddress8(LIGHTSENSOR_I2C_ADDRESS,LIGHTSENSOR_I2C_REGISTER_CONFIG_BANK , 0x80);			//0x80 = Bank1 (x,y,z,NIR) 0x00 = Bank0 (x,y,b,b)
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


