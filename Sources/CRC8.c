/*
 * CRC8.c
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 *
 *      CRC8 implementation, copied from
 *      https://www.mikrocontroller.net/attachment/highlight/61519
 */

#include "CRC8.h"

#define CRC8_POLYNOM (0x07)

static uint8_t crc8_bytecalc(unsigned char byte, uint8_t* seed)
{
	uint8_t i;
	uint8_t flag;
	uint8_t polynom = CRC8_POLYNOM;

	for (i = 0; i < 8; i++)
	{
		if (*seed & 0x80)
		{
			flag = 1;
		}
		else
		{
			flag = 0;
		}
		*seed <<= 1;
		if (byte & 0x80)
		{
			*seed |= 1;
		}
		byte <<= 1;
		if (flag)
		{
			*seed ^= polynom;
		}
	}
	return *seed;
}

static uint8_t crc8_messagecalc(unsigned char *msg, uint8_t len, uint8_t* seed)
{
  for(int i=0; i<len; i++)
  {
    crc8_bytecalc(msg[i],seed);
  }
  uint8_t crc = crc8_bytecalc(0,seed);
  return crc;
}

uint8_t crc8_SDEPcrc(SDEPmessage_t *message)
{
	uint8_t crc,seed = 0;

	crc = crc8_bytecalc(message->type, &seed);
	crc = crc8_bytecalc((uint8_t)message->cmdId, &seed);
	crc = crc8_bytecalc((uint8_t)(message->cmdId>>8), &seed);
	crc = crc8_bytecalc(message->payloadSize, &seed);
	crc = crc8_messagecalc((unsigned char*)message->payload, message->payloadSize, &seed);
	return crc;
}

void crc8_liDoSample(liDoSample_t *sample)
{
	uint8_t crc,seed = 0;

	crc = crc8_bytecalc((uint8_t)sample->unixTimeStamp,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->unixTimeStamp>>8),&seed);
	crc = crc8_bytecalc((uint8_t)(sample->unixTimeStamp>>16),&seed);
	crc = crc8_bytecalc((uint8_t)(sample->unixTimeStamp>>24),&seed);
	crc = crc8_bytecalc((uint8_t)sample->lightChannelX,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->lightChannelX>>8),&seed);
	crc = crc8_bytecalc((uint8_t)sample->lightChannelY,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->lightChannelY>>8),&seed);
	crc = crc8_bytecalc((uint8_t)sample->lightChannelZ,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->lightChannelZ>>8),&seed);
	crc = crc8_bytecalc((uint8_t)sample->lightChannelIR,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->lightChannelIR>>8),&seed);
	crc = crc8_bytecalc((uint8_t)sample->lightChannelB440,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->lightChannelB440>>8),&seed);
	crc = crc8_bytecalc((uint8_t)sample->lightChannelB490,&seed);
	crc = crc8_bytecalc((uint8_t)(sample->lightChannelB490>>8),&seed);
	crc = crc8_bytecalc((uint8_t)sample->accelX,&seed);
	crc = crc8_bytecalc((uint8_t)sample->accelY,&seed);
	crc = crc8_bytecalc((uint8_t)sample->accelZ,&seed);
	crc = crc8_bytecalc((uint8_t)sample->temp,&seed);
	crc = crc8_bytecalc(0,&seed);
	sample->crc = crc;
}
