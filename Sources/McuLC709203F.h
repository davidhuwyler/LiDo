/*
 * LC709203F.h
 *
 *  Created on: 06.06.2019
 *      Author: Sarah Gander
 */

#ifndef SOURCES_MCULC709203F_H_
#define SOURCES_MCULC709203F_H_

uint16_t McuLC_GetVoltage(void);
int16_t McuLC_GetTemperature(void);
uint16_t McuLC_GetRSOC(void);
uint16_t McuLC_GetITE(void);
uint16_t McuLC_GetICversion(void);

typedef enum {
  McuLC_CURRENT_DIR_AUTO,
  McuLC_CURRENT_DIR_CHARGING,
  McuLC_CURRENT_DIR_DISCHARING,
  McuLC_CURRENT_DIR_ERROR /* error case */
} McuLC_CurrentDirection;

McuLC_CurrentDirection LC_GetCurrentDirection(void);
McuLC_CurrentDirection LC_SetCurrentDirection(void);

#include "CLS1.h"
uint8_t McuLC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

void McuLC_Wakeup(void); /* must be done before any other I2C communication on the bus! */

void McuLC_Init(void);

void McuLC_Deinit(void);

#endif /* SOURCES_MCULC709203F_H_ */
