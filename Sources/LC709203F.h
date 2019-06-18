/*
 * LC709203F.h
 *
 *  Created on: 06.06.2019
 *      Author: Sarah Gander
 */

#ifndef SOURCES_LC709203F_H_
#define SOURCES_LC709203F_H_


uint16_t LCgetVoltage(void);
int16_t LCgetTemp(void);
uint16_t LCgetRSOC(void);
uint16_t LCgetITE(void);
uint16_t LCgetICversion(void);

typedef enum {
  LC_CURRENT_DIR_AUTO,
  LC_CURRENT_DIR_CHARGING,
  LC_CURRENT_DIR_DISCHARING,
  LC_CURRENT_DIR_ERROR /* error case */
} LC_CurrentDirection;

LC_CurrentDirection LCgetCurrentDirection(void);

#include "CLS1.h"
uint8_t LC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

void LC_Wakeup(void); /* must be done before any other I2C communication on the bus! */

void LC_Init(void);

#endif /* SOURCES_LC709203F_H_ */
