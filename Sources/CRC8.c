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
#include "CS1.h"

uint8_t crc8_bytecalc(unsigned char byte,uint8_t* seed)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
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
	CS1_ExitCritical();
	return *seed;
}

uint8_t crc8_messagecalc(unsigned char *msg, uint8_t len,uint8_t* seed)
{
  CS1_CriticalVariable();
  CS1_EnterCritical();

  for(int i=0; i<len; i++)
  {
    crc8_bytecalc(msg[i],seed);
  }
  uint8_t crc = crc8_bytecalc(0,seed);
  CS1_ExitCritical();
  return crc;
}

unsigned char CRC8(unsigned char crc, unsigned char ch)
{
	bool bpop;

	for (int i = 0; i < 8; i++)
	{
		if (crc & 0x80) // if the H.O. bit of crc is set, then the left shift will cause a pop
			bpop = TRUE;
		else
			bpop = FALSE;

		crc <<= 1;                    // left shift - divide crc by two

		if (ch & 0x80)            	  // if the H.O. bit of the 'register' is set
		{
			crc |= 0x01;              // set the L.O. bit of crc
		}
		else
		{
			crc &= 0xfe;              // clear the L.O. bit of CRC
		}

		if (bpop)             		  // if we poped a bit off the crc at the last shift
			crc = crc ^ CRC8_POLYNOM; // XOR crc polynomial

		ch <<= 1;                     // shift the 'register' by one bit
	}

	return crc;
}




