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

int LCgetVoltage(void);
int LCgetTemp(void);
int LCgetRSOC(void);
int LCgetITE(void);

void LC_Init(void);

#endif /* SOURCES_LC709203F_H_ */
