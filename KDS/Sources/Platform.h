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

#if defined(PEcfg_V2_0)
  #define PL_BOARD_REV      20  /* V2.0 */
#elif defined(PEcfg_V2_1)
  #define PL_BOARD_REV      21  /* V2.1 */
#elif defined(PEcfg_V2_2)
  #define PL_BOARD_REV      22  /* V2.2 */
#endif

/* Sensor configurations */
#define PL_CONFIG_HAS_LIGHT_SENSOR   (0)
#define PL_CONFIG_HAS_ACCEL_SENSOR   (1)
#define PL_CONFIG_HAS_GAUGE_SENSOR   (1 && (PL_BOARD_REV==21||PL_BOARD_REV==22)) /* LC709203F on V2.1. Earlier board had an ADC */
#define PL_CONFIG_HAS_BATT_ADC       (0 && PL_BOARD_REV==20)
#define PL_CONFIG_HAS_SW_RTC         (0) /* disabled, as we use HW RTC. SW RTC would require a FreeRTOS timer if using tickless idle mode */
#define PL_CONFIG_HAS_SPIF_PWR_PIN   (1 && (PL_BOARD_REV==20 || PL_BOARD_REV==21)) /* V2.2 does not have ability to power of Flash chip */
#define PL_CONFIG_HAS_WATCHDOG       (1) /* disable for better debugging only! */

#if PL_BOARD_REV==20 || PL_BOARD_REV==21
  /* User push button (PTC1) is HIGH active */
  #define USER_BUTTON_PRESSED()   ((GPIOC_PDIR&(1<<1))!=0)
#elif PL_BOARD_REV==22
  /* User push button (PTD0) is HIGH active */
  #define USER_BUTTON_PRESSED()   ((GPIOD_PDIR&(1<<0))!=0)
#else
  #error "unknown board"
#endif

// Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
//     exception return operation might vector to incorrect interrupt
#define __DSB() {  __asm volatile ("dsb 0xF":::"memory"); }

#endif /* SOURCES_PLATFORM_H_ */
