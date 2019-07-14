/* ###################################################################
**     Filename    : main.c
**     Project     : LiDo_CustomPCB
**     Processor   : MK22DX256VLF5
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-04-24, 17:21, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
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
#include "SYS1.h"
#include "RTT1.h"
#include "WDog1.h"
#include "WatchDogLdd1.h"
#include "INT_RTC.h"
#include "PIN_SENSOR_PWR.h"
#include "BitIoLdd6.h"
#include "GI2C1.h"
#include "CI2C1.h"
#include "PIN_SPIF_PWR.h"
#include "BitIoLdd2.h"
#include "PIN_SPIF_RESET.h"
#include "BitIoLdd3.h"
#include "PIN_SPIF_CS.h"
#include "BitIoLdd4.h"
#include "PIN_SPIF_WP.h"
#include "BitIoLdd5.h"
#include "SM1.h"
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
#include "LED_G.h"
#include "LEDpin2.h"
#include "BitIoLdd7.h"
#include "LED_B.h"
#include "LEDpin3.h"
#include "BitIoLdd8.h"
#include "PTC.h"
#include "PIN_POWER_ON.h"
#include "BitIoLdd9.h"
#include "PIN_PS_MODE.h"
#include "BitIoLdd12.h"
#include "I2C_SDA.h"
#include "BitIoLdd17.h"
#include "I2C_SCL.h"
#include "BitIoLdd18.h"
#include "BAT_ALARM.h"
#include "BitIoLdd19.h"
#include "RESET_AS7264.h"
#include "BitIoLdd21.h"
#include "PIN_PWR_CHARGE_STATE.h"
#include "BitIoLdd11.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
/* User includes (#include below this line is not maintained by Processor Expert) */
#include "Application.h"
/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  /* Write your code here */
  APP_Run();

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
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
