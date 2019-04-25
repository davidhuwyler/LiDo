/* ###################################################################
**     Filename    : Events.h
**     Project     : LiDo_CustomPCB
**     Processor   : MK22DX256VLF5
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-04-24, 17:21, # CodeGen: 0
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Contents    :
**         Cpu_OnNMIINT - void Cpu_OnNMIINT(void);
**
** ###################################################################*/
/*!
** @file Events.h
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup Events_module Events module documentation
**  @{
*/         

#ifndef __Events_H
#define __Events_H
/* MODULE Events */

#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "FRTOS1.h"
#include "TmDt1.h"
#include "UTIL1.h"
#include "CLS1.h"
#include "MCUC1.h"
#include "LED_R.h"
#include "LEDpin1.h"
#include "BitIoLdd1.h"
#include "WAIT1.h"
#include "XF1.h"
#include "CS1.h"
#include "KIN1.h"
#include "RTC1.h"
#include "HF1.h"
#include "AS1.h"
#include "ASerialLdd1.h"
#include "WDog1.h"
#include "WatchDogLdd1.h"
#include "INT_RTC.h"
#include "GI2C1.h"
#include "CI2C1.h"
#include "TMOUT1.h"
#include "PIN_SPIF_PWR.h"
#include "BitIoLdd2.h"
#include "PIN_SPIF_RESET.h"
#include "BitIoLdd3.h"
#include "PIN_SPIF_CS.h"
#include "BitIoLdd4.h"
#include "PIN_SPIF_WP.h"
#include "BitIoLdd5.h"
#include "SM1.h"
#include "SMasterLdd1.h"
#include "USB1.h"
#include "CDC1.h"
#include "Tx1.h"
#include "Rx1.h"
#include "USB0.h"
#include "SDEPtoShellBuf.h"
#include "ShelltoSDEPBuf.h"
#include "FileToSDEPBuf.h"
#include "SDEPpendingAlertsBuffer.h"
#include "PTB.h"
#include "PTC.h"

#ifdef __cplusplus
extern "C" {
#endif 

/*
** ===================================================================
**     Event       :  Cpu_OnNMIINT (module Events)
**
**     Component   :  Cpu [MK22DN512MC5]
*/
/*!
**     @brief
**         This event is called when the Non maskable interrupt had
**         occurred. This event is automatically enabled when the [NMI
**         interrupt] property is set to 'Enabled'.
*/
/* ===================================================================*/
void Cpu_OnNMIINT(void);


void FRTOS1_vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
/*
** ===================================================================
**     Event       :  FRTOS1_vApplicationStackOverflowHook (module Events)
**
**     Component   :  FRTOS1 [FreeRTOS]
**     Description :
**         if enabled, this hook will be called in case of a stack
**         overflow.
**     Parameters  :
**         NAME            - DESCRIPTION
**         pxTask          - Task handle
**       * pcTaskName      - Pointer to task name
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS1_vApplicationTickHook(void);
/*
** ===================================================================
**     Event       :  FRTOS1_vApplicationTickHook (module Events)
**
**     Component   :  FRTOS1 [FreeRTOS]
**     Description :
**         If enabled, this hook will be called by the RTOS for every
**         tick increment.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS1_vApplicationIdleHook(void);
/*
** ===================================================================
**     Event       :  FRTOS1_vApplicationIdleHook (module Events)
**
**     Component   :  FRTOS1 [FreeRTOS]
**     Description :
**         If enabled, this hook will be called when the RTOS is idle.
**         This might be a good place to go into low power mode.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS1_vApplicationMallocFailedHook(void);
/*
** ===================================================================
**     Event       :  FRTOS1_vApplicationMallocFailedHook (module Events)
**
**     Component   :  FRTOS1 [FreeRTOS]
**     Description :
**         If enabled, the RTOS will call this hook in case memory
**         allocation failed.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS1_vOnPreSleepProcessing(portTickType expectedIdleTicks);
/*
** ===================================================================
**     Event       :  FRTOS1_vOnPreSleepProcessing (module Events)
**
**     Component   :  FRTOS1 [FreeRTOS]
**     Description :
**         Used in tickless idle mode only, but required in this mode.
**         Hook for the application to enter low power mode.
**     Parameters  :
**         NAME            - DESCRIPTION
**         expectedIdleTicks - expected idle
**                           time, in ticks
**     Returns     : Nothing
** ===================================================================
*/

/* END Events */

#ifdef __cplusplus
}  /* extern "C" */
#endif 

#endif 
/* ifndef __Events_H*/
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
