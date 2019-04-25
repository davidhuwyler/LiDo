/*
 * PowerManagement.h
 *
 *  Created on: Apr 25, 2019
 *      Author: dave
 */

#ifndef SOURCES_POWERMANAGEMENT_H_
#define SOURCES_POWERMANAGEMENT_H_

#include "Platform.h"

void PowerManagement_init(void);
uint16_t PowerManagement_getBatteryVoltage(void);

#endif /* SOURCES_POWERMANAGEMENT_H_ */
