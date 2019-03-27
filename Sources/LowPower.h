/*
 * LowPower.h
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */

#ifndef SOURCES_LOWPOWER_H_
#define SOURCES_LOWPOWER_H_

//#include <stdbool.h>

void LowPower_EnterLowpowerMode(void);
void LowPower_EnableStopMode(void);
void LowPower_DisableStopMode(void);
bool LowPower_StopModeIsEnabled(void);

#endif /* SOURCES_LOWPOWER_H_ */
