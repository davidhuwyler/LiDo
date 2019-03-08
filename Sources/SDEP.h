/*
 * SDEP.h
 *
 *  Created on: Mar 4, 2019
 *      Author: dave
 */

#ifndef SOURCES_SDEP_H_
#define SOURCES_SDEP_H_

#include <stdint.h>
#include <stddef.h>
#include "CLS1.h"

#define SDEP_MESSAGE_MAX_NOF_BYTES 131

uint8_t SDEP_Parse(void);
uint8_t SDEP_Init(void);

#endif /* SOURCES_SDEP_H_ */
