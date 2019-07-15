/*
 * PowerManagement.h
 *
 *  Created on: Apr 25, 2019
 *      Author: dave
 */

#ifndef SOURCES_POWERMANAGEMENT_H_
#define SOURCES_POWERMANAGEMENT_H_

#include "Platform.h"

#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
  uint8_t PowerManagement_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
#endif

bool PowerManagement_IsCharging(void);
void PowerManagement_PowerOn(void);
void PowerManagement_PowerOff(void);
void PowerManagement_init(void);
void PowerManagement_ResumeTaskIfNeeded(void);
uint16_t PowerManagement_getBatteryVoltage(void);

#endif /* SOURCES_POWERMANAGEMENT_H_ */
