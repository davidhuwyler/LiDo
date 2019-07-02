/*
 * CRC8.h
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 */

#ifndef SOURCES_CRC8_H_
#define SOURCES_CRC8_H_

#include "Platform.h"
#include "Application.h"
#include "SDEP.h"

uint8_t crc8_SDEPcrc(SDEPmessage_t* message);
void crc8_liDoSample(liDoSample_t* sample);

#endif /* SOURCES_CRC8_H_ */
