/*
 * CRC8.h
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 */

#ifndef SOURCES_CRC8_H_
#define SOURCES_CRC8_H_

#include <stdint.h>
#include <stddef.h>

#define CRC8_POLYNOM (0x07)

uint8_t crc8_bytecalc(unsigned char byte,uint8_t* seed);
uint8_t crc8_messagecalc(unsigned char *msg, uint8_t len,uint8_t* seed);
unsigned char CRC8(unsigned char crc, unsigned char ch)

#endif /* SOURCES_CRC8_H_ */
