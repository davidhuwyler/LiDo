/*
 * Platform.h
 *
 *  Created on: Mar 20, 2019
 *      Author: Dave
 */

#ifndef SOURCES_PLATFORM_H_
#define SOURCES_PLATFORM_H_

#include "PE_Types.h"
#include "Cpu.h"

#if defined(PEcfg_V2_1)
#define PL_BOARD_REV      21  /* V2.1 */
#elif defined(PEcfg_V2_0)
#define PL_BOARD_REV      20  /* V2.0 */
#endif

/* Sensor configurations */
#define PL_CONFIG_HAS_LIGHT_SENSOR   (0)
#define PL_CONFIG_HAS_ACCEL_SENSOR   (0)
#define PL_CONFIG_HAS_GAUGE_SENSOR   (1 && PL_BOARD_REV==21) /* LC709203F on V2.1. Earlier board had an ADC */
#define PL_CONFIG_HAS_BATT_ADC       (1 && PL_BOARD_REV==20)

//Inputs
#define USER_BUTTON_PRESSED ((GPIOC_PDIR & 0x2)>>1)


// Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
//     exception return operation might vector to incorrect interrupt
#define __DSB() {  __asm volatile ("dsb 0xF":::"memory"); }

#endif /* SOURCES_PLATFORM_H_ */
