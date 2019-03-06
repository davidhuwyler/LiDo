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

uint8_t SDEP_Init(void);
uint8_t SDEP_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
uint8_t SDEP_ParseSilentCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

#endif /* SOURCES_SDEP_H_ */
