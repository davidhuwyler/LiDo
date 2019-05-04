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
#include "RTC.h"
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

static TaskHandle_t shellTaskHandle;
static TickType_t shellEnabledTimestamp;
static bool shellDisablingRequest = FALSE;
static bool shellDisablingIsInitiated = FALSE;

static const CLS1_ParseCommandCallback CmdParserTable[] =
{
  CLS1_ParseCommand,
  KIN1_ParseCommand,
  LightSensor_ParseCommand,
  AccelSensor_ParseCommand,
  RTC_ParseCommand,
  TmDt1_ParseCommand,
  SPIF_ParseCommand,
  FS_ParseCommand,
  AppData_ParseCommand,
  ErrorLogFile_ParseCommand,
  APP_ParseCommand,
  NULL /* sentinel */
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
  (void)param;
  TickType_t xLastWakeTime;
  shellEnabledTimestamp = xTaskGetTickCount();
  unsigned short charsInUARTbuf;
  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  CS1_CriticalVariable();
	  CS1_EnterCritical();

	  //Disable Shell if Requested (button or SDEP) or if Shell runns already longer than 10s and USB_CDC is disconnected
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
	  CLS1_ReadAndParseWithCommandTable(CLS1_DefaultShellBuffer, sizeof(CLS1_DefaultShellBuffer), CLS1_GetStdio(), CmdParserTable);
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
  CLS1_DefaultShellBuffer[0] = '\0';
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
