/*
 * PowerManagement.h
 *
 *  Created on: Apr 25, 2019
 *      Author: dave
 */

#ifndef SOURCES_POWERMANAGEMENT_H_
#define SOURCES_POWERMANAGEMENT_H_

#include "Platform.h"

#include "CLS1.h"
uint8_t PowerManagement_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

void PowerManagement_init(void);
void PowerManagement_ResumeTaskIfNeeded(void);
uint16_t PowerManagement_getBatteryVoltage(void);

#endif /* SOURCES_POWERMANAGEMENT_H_ */
