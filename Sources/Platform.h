/*
 * Platform.h
 *
 *  Created on: Mar 20, 2019
 *      Author: Dave
 */

#ifndef SOURCES_PLATFORM_H_
#define SOURCES_PLATFORM_H_

#include "PE_Types.h"
#include "CPU.h"

/* Sensor configurations */

#define PL_CONFIG_HAS_LIGHT_SENSOR   (1)
#define PL_CONFIG_HAS_ACCEL_SENSOR   (1)

//Inputs
#define USER_BUTTON_PRESSED ((GPIOC_PDIR & 0x2)>>1)


// Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
//     exception return operation might vector to incorrect interrupt
#define __DSB() {  __asm volatile ("dsb 0xF":::"memory"); }

#endif /* SOURCES_PLATFORM_H_ */
