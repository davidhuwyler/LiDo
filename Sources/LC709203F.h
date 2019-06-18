/*
 * LC709203F.h
 *
 *  Created on: 06.06.2019
 *      Author: Sarah Gander
 */

#ifndef SOURCES_LC709203F_H_
#define SOURCES_LC709203F_H_

void LCwakeup(void);
void LCinit(void);

uint16_t LCgetVoltage(void);
int16_t LCgetTemp(void);
uint16_t LCgetRSOC(void);
uint16_t LCgetITE(void);
uint16_t LCgetICversion(void);

#include "CLS1.h"
uint8_t LC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

void LC_Init(void);

#endif /* SOURCES_LC709203F_H_ */
