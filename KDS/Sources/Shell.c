/*
 * Shell.c
 *
 *  Created on: Feb 21, 2019
 *      Author: dave
 */
#include "Platform.h"

#include "Application.h"
#include "Shell.h"
#include "CLS1.h"
#include "KIN1.h"
#include "LightSensor.h"
#include "AccelSensor.h"
#include "LowPower.h"
#include "SPIF.h"
#include "UTIL1.h"
#include "FileSystem.h"
#include "AppDataFile.h"
#include "TmDt1.h"
#include "WAIT1.h"
#include "SDEP.h"
#include "SDEPioHandler.h"
#include "UI.h"
#include "USB1.h"
#include "CDC1.h"
#include "ErrorLogFile.h"
#include "RTT1.h"
#include "McuLC709203F.h"
#include "AS1.h"
#include "PowerManagement.h"

#define SHELL_CONFIG_HAS_SHELL_EXTRA_UART  (1)
#define SHELL_CONFIG_HAS_SHELL_EXTRA_RTT   (1)
#define SHELL_CONFIG_HAS_SHELL_EXTRA_CDC   (0)

static TaskHandle_t shellTaskHandle;
static TickType_t shellEnabledTimestamp;
static bool shellDisablingRequest = FALSE;
static bool shellDisablingIsInitiated = FALSE;

static const CLS1_ParseCommandCallback CmdParserTable[] =
{
  CLS1_ParseCommand,
  FRTOS1_ParseCommand,
  KIN1_ParseCommand,
#if PL_CONFIG_HAS_LIGHT_SENSOR
  LightSensor_ParseCommand,
#endif
#if PL_CONFIG_HAS_ACCEL_SENSOR
  AccelSensor_ParseCommand,
#endif
#if PL_CONFIG_HAS_GAUGE_SENSOR
  McuLC_ParseCommand,
#endif
  PowerManagement_ParseCommand,
  TmDt1_ParseCommand,
  SPIF_ParseCommand,
  FS_ParseCommand,
  AppData_ParseCommand,
  ErrorLogFile_ParseCommand,
  APP_ParseCommand,
  NULL /* sentinel */
};

typedef struct {
  CLS1_ConstStdIOType *stdio;
  unsigned char *buf;
  size_t bufSize;
} SHELL_IODesc;

/*--------------------------------------------------------------------------*/
#if SHELL_CONFIG_HAS_SHELL_EXTRA_UART
  static bool UART_KeyPressed(void) {
    return AS1_GetCharsInRxBuf()!=0;
  }

  static void UART_SendChar(uint8_t ch) {
    uint8_t res;
    int timeoutMs = 5;


    do {
      res = AS1_SendChar((uint8_t)ch);  /* Send char */
      if (res==ERR_TXFULL) {
        WAIT1_WaitOSms(1);
      }
      if(timeoutMs<=0) {
        break; /* timeout */
      }
      timeoutMs -= 1;
    } while(res==ERR_TXFULL);
  }

  static void UART_ReceiveChar(uint8_t *p) {
    if (AS1_RecvChar(p)!=ERR_OK) {
      *p = '\0';
    }
  }

  static CLS1_ConstStdIOType UART_stdio = {
    .stdIn = UART_ReceiveChar,
    .stdOut = UART_SendChar,
    .stdErr = UART_SendChar,
    .keyPressed = UART_KeyPressed,
  };

  static uint8_t UART_DefaultShellBuffer[CLS1_DEFAULT_SHELL_BUFFER_SIZE]; /* default buffer which can be used by the application */
#endif /* SHELL_CONFIG_HAS_SHELL_EXTRA_UART */
/*--------------------------------------------------------------------------*/

static void SHELL_ReadChar(uint8_t *p) {
  *p = '\0'; /* default, nothing available */
#if SHELL_CONFIG_HAS_SHELL_EXTRA_RTT
  if (RTT1_stdio.keyPressed()) {
    RTT1_stdio.stdIn(p);
    return;
  }
#endif
#if SHELL_CONFIG_HAS_SHELL_EXTRA_UART
  if (UART_stdio.keyPressed()) {
    UART_stdio.stdIn(p);
    return;
  }
#endif
#if SHELL_CONFIG_HAS_SHELL_EXTRA_CDC
  if (CDC1_stdio.keyPressed()) {
    CDC1_stdio.stdIn(p);
    return;
  }
#endif
}


static void SHELL_SendChar(uint8_t ch) {
#if SHELL_CONFIG_HAS_SHELL_EXTRA_UART
  UART_SendChar(ch);
#endif
#if SHELL_CONFIG_HAS_SHELL_EXTRA_CDC
  CDC1_SendChar(ch); /* copy on CDC */
#endif
#if SHELL_CONFIG_HAS_SHELL_EXTRA_RTT
  RTT1_SendChar(ch); /* copy on RTT */
#endif
}

/* copy on other I/Os */
CLS1_ConstStdIOType SHELL_stdio =
{
  (CLS1_StdIO_In_FctType)SHELL_ReadChar, /* stdin */
  (CLS1_StdIO_OutErr_FctType)SHELL_SendChar, /* stdout */
  (CLS1_StdIO_OutErr_FctType)SHELL_SendChar, /* stderr */
  CLS1_KeyPressed /* if input is not empty */
};

CLS1_ConstStdIOType *SHELL_GetStdio(void) {
  return &SHELL_stdio;
}


static const SHELL_IODesc ios[] =
{
#if SHELL_CONFIG_HAS_SHELL_EXTRA_RTT
  {&RTT1_stdio, RTT1_DefaultShellBuffer, sizeof(RTT1_DefaultShellBuffer)},
#endif
#if SHELL_CONFIG_HAS_SHELL_EXTRA_UART
  {&UART_stdio, UART_DefaultShellBuffer, sizeof(UART_DefaultShellBuffer)},
#endif
#if SHELL_CONFIG_HAS_SHELL_EXTRA_CDC
  {&CDC1_stdio, CDC1_DefaultShellBuffer, sizeof(CDC1_DefaultShellBuffer)},
#endif
};


static void SHELL_SwitchIOifNeeded(void)
{
	static TickType_t SDEPioTimer;
	static bool SDEPioTimerStarted = FALSE;
	if(SDEPio_NewSDEPmessageAvail())
	{
		SDEPioTimerStarted = TRUE;
		SDEPioTimer = xTaskGetTickCount();
	}
	else if(SDEPioTimerStarted && xTaskGetTickCount() - SDEPioTimer > pdMS_TO_TICKS(300))
	{
		SDEPio_switchIOtoStdIO();
		SDEPioTimerStarted = FALSE;
	}
}

//Function needs to be called 3times to
//Disable the shell. This makes sure, the
//Answer message got out before disabling
//the Shell
static void SHELL_Disable(void)
{
	static uint8_t cnt = 0;
	shellDisablingIsInitiated = TRUE;
	if(cnt==2)
	{
		UI_StopShellIndicator();
		CDC1_Deinit();
		USB1_Deinit();

		//Switch Peripheral Clock from ICR48 to FLL clock (Datasheet p.265)
		//More Infos in AN4905 Crystal-less USB operation on Kinetis MCUs
		//SIM_SOPT2 &= ~SIM_SOPT2_PLLFLLSEL_MASK;
		//SIM_SOPT2 |= SIM_SOPT2_PLLFLLSEL(0x0);
		//USB0_CLK_RECOVER_IRC_EN = 0x0;	//Disable USB Clock (IRC 48MHz)

		LowPower_EnableStopMode();
		vTaskSuspend(shellTaskHandle);
	}
	else
	{
		cnt++;
	}
}

static void SHELL_task(void *param) {
  TickType_t xLastWakeTime;
  shellEnabledTimestamp = xTaskGetTickCount();
  unsigned short charsInUARTbuf;
  int i;

  (void)param;
  /* initialize buffers */
  for(i=0;i<sizeof(ios)/sizeof(ios[0]);i++) {
    ios[i].buf[0] = '\0';
  }
  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  CS1_CriticalVariable();
	  CS1_EnterCritical();

	  //Disable Shell if Requested (button or SDEP) or if Shell runs already longer than 10s and USB_CDC is disconnected
	  if(	shellDisablingRequest ||
	     (( xTaskGetTickCount()-shellEnabledTimestamp > SHELL_MIN_ENABLE_TIME_AFTER_BOOT_MS ) && CDC1_ApplicationStarted() == FALSE) ||
		 	shellDisablingIsInitiated)
	  {
		  CS1_ExitCritical();
		  SHELL_Disable();
	  }
	  else
	  {
		  CS1_ExitCritical();
	  }

	  SHELL_SwitchIOifNeeded();
	  SDEP_Parse();
    /* process all I/Os */
    for(i=0;i<sizeof(ios)/sizeof(ios[0]);i++) {
      (void)CLS1_ReadAndParseWithCommandTable(ios[i].buf, ios[i].bufSize, ios[i].stdio, CmdParserTable);
    }
	  SDEPio_HandleShellCMDs();
	  SDEP_SendPendingAlert();
	  while(SDEPio_HandleFileCMDs(0) != ERR_RXEMPTY){}

//	  if(SDEPio_HandleFileCMDs(0) == ERR_RXEMPTY) //Only Wait if there is no file to transfer
//	  {
//		  vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
//	  }
	  vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
  } /* for */
}

void SHELL_Init(void) {
  CLS1_SetStdio(&SHELL_stdio); /* make sure that Shell is using our custom I/O handler */

  PORTB_PCR16 |= 0x3;  //Enable Pullup UART RX
  PORTB_PCR17 |= 0x3;  //Enable Pullup UART TX

  if (xTaskCreate(SHELL_task, "Shell", 3000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, &shellTaskHandle) != pdPASS)
  {
	  for(;;){} /* error! probably out of memory */
  }
}

void SHELL_requestDisabling(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	shellDisablingRequest = TRUE;
	CS1_ExitCritical();
}
