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


//Inputs
#define USER_BUTTON_PRESSED !(GPIOB_PDIR & 0x1)


// Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
//     exception return operation might vector to incorrect interrupt
#define __DSB() {  __asm volatile ("dsb 0xF":::"memory"); }

#endif /* SOURCES_PLATFORM_H_ */
