/*
 * LowPower.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */
#include "LowPower.h"
#include "Cpu.h"

void LowPower_EnterLowpowerMode(void)
{
	Cpu_SetOperationMode(DOM_SLEEP,NULL,NULL);
}

