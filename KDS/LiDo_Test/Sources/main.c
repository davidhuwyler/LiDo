/* ###################################################################
**     Filename    : main.c
**     Project     : LiDo_Test
**     Processor   : MK22DX256VMC5
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-07-15, 10:04, # CodeGen: 0
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
#include "WAIT1.h"
#include "MCUC1.h"
#include "LED_R.h"
#include "LEDpin1.h"
#include "BitIoLdd1.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "LED_G.h"
#include "LEDpin2.h"
#include "BitIoLdd2.h"
#include "LED_B.h"
#include "LEDpin3.h"
#include "BitIoLdd3.h"
#include "PIN_POWER_ON.h"
#include "BitIoLdd4.h"
#include "GI2C1.h"
#include "CI2C1.h"
#include "I2C_SDA.h"
#include "BitIoLdd11.h"
#include "I2C_SCL.h"
#include "BitIoLdd12.h"
#include "UTIL1.h"
#include "PIN_SPIF_PWR.h"
#include "BitIoLdd5.h"
#include "PIN_PS_MODE.h"
#include "BitIoLdd6.h"
#include "TU1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
/* User includes (#include below this line is not maintained by Processor Expert) */

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */
  int i;

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  TI1_Disable();
  // PIN_POWER_ON: HIGH
  // 4.15ma
  // DO_SPIF_PWR HIGH (flash off)
  // 4.26ma?
  // PIN_PS_MODE (DC-DC power safe pin) LOW */
  // 230uA
  for(i=0;i<10;i++) {
    LED_R_On();
    LED_G_On();
    LED_B_On();
    WAIT1_Waitms(10);
    LED_R_Off();
    LED_G_Off();
    LED_B_Off();
    WAIT1_Waitms(1000);
   }
  TI1_Enable(); /* enable LPTMR0 */

  //PIN_PS_MODE_SetVal(); /* low power DC-DC */
  //PIN_PS_MODE_ClrVal(); /* low power DC-DC */

  for(;;) {
    Cpu_SetOperationMode(DOM_STOP, NULL, NULL);
  }
  for(;;) {
    __asm("nop");
  }
  /* For example: for(;;) { } */

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
