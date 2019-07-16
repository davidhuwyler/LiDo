/* ###################################################################
**     Filename    : Events.h
**     Project     : LiDo_Test
**     Processor   : MK22DX256VMC5
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-07-15, 10:04, # CodeGen: 0
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
#include "WAIT1.h"
#include "MCUC1.h"
#include "LED_R.h"
#include "LEDpin1.h"
#include "BitIoLdd1.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "LED_G.h"
#include "LEDpin5.h"
#include "BitIoLdd8.h"
#include "LED_B.h"
#include "LEDpin4.h"
#include "BitIoLdd7.h"
#include "PIN_POWER_ON.h"
#include "BitIoLdd9.h"
#include "GI2C1.h"
#include "CI2C1.h"
#include "I2C_SDA.h"
#include "BitIoLdd11.h"
#include "I2C_SCL.h"
#include "BitIoLdd12.h"
#include "UTIL1.h"
#include "INT_LI_DONE.h"
#include "ExtIntLdd2.h"
#include "RES_OPT.h"
#include "BitIoLdd13.h"
#include "PIN_SPIF_RESET.h"
#include "BitIoLdd14.h"
#include "PIN_SPIF_CS.h"
#include "BitIoLdd15.h"
#include "PIN_SPIF_WP.h"
#include "BitIoLdd16.h"
#include "SM1.h"
#include "SYS1.h"
#include "RTT1.h"
#include "CLS1.h"
#include "CS1.h"
#include "XF1.h"
#include "PIN_PS_MODE.h"
#include "BitIoLdd10.h"
#include "TU1.h"

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


/*
** ===================================================================
**     Event       :  TI1_OnInterrupt (module Events)
**
**     Component   :  TI1 [TimerInt_LDD]
*/
/*!
**     @brief
**         Called if periodic event occur. Component and OnInterrupt
**         event must be enabled. See [SetEventMask] and [GetEventMask]
**         methods. This event is available only if a [Interrupt
**         service/event] is enabled.
**     @param
**         UserDataPtr     - Pointer to the user or
**                           RTOS specific data. The pointer passed as
**                           the parameter of Init method.
*/
/* ===================================================================*/
void TI1_OnInterrupt(void);

/*
** ===================================================================
**     Event       :  Cpu_OnLLSWakeUpINT (module Events)
**
**     Component   :  Cpu [MK22DN512MC5]
*/
/*!
**     @brief
**         This event is called when Low Leakage WakeUp interrupt
**         occurs. LLWU flags indicating source of the wakeup can be
**         obtained by calling the [GetLLSWakeUpFlags] method. Flags
**         indicating the external pin wakeup source are automatically
**         cleared after this event is executed. It is responsibility
**         of user to clear flags corresponding to internal modules.
**         This event is automatically enabled when [LLWU interrupt
**         request] is enabled.
*/
/* ===================================================================*/
void Cpu_OnLLSWakeUpINT(void);

void INT_LI_DONE_OnInterrupt(void);
/*
** ===================================================================
**     Event       :  INT_LI_DONE_OnInterrupt (module Events)
**
**     Component   :  INT_LI_DONE [ExtInt]
**     Description :
**         This event is called when an active signal edge/level has
**         occurred.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

/*
** ===================================================================
**     Event       :  SM1_OnBlockReceived (module Events)
**
**     Component   :  SM1 [SPIMaster_LDD]
*/
/*!
**     @brief
**         This event is called when the requested number of data is
**         moved to the input buffer. This method is available only if
**         the ReceiveBlock method is enabled.
**     @param
**         UserDataPtr     - Pointer to the user or
**                           RTOS specific data. The pointer is passed
**                           as the parameter of Init method. 
*/
/* ===================================================================*/
void SM1_OnBlockReceived(LDD_TUserData *UserDataPtr);

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
