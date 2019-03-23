/*
 * Shell.c
 *
 *  Created on: Feb 21, 2019
 *      Author: dave
 */
#include "Platform.h"

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

static TaskHandle_t shellTaskHandle;
static TickType_t shellEnabledTimestamp;
static bool shellDisablingRequest = FALSE;

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
	if(cnt==2)
	{
		UI_StopShellIndicator();
		CDC1_Deinit();
		USB1_Deinit();
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
  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  //SPIF_ReleaseFromDeepPowerDown();
	  CS1_CriticalVariable();
	  CS1_EnterCritical();
	  if(shellDisablingRequest)
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
	  SDEPio_HandleFileCMDs(0);
	  SDEP_SendPendingAlert();
	  //SPIF_GoIntoDeepPowerDown();
	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(10));
  } /* for */
}

void SHELL_Init(void) {
  CLS1_DefaultShellBuffer[0] = '\0';
  if (xTaskCreate(SHELL_task, "Shell", 5000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, &shellTaskHandle) != pdPASS)
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

//void SHELL_enable(void)
//{
//	FRTOS1_xTaskResumeFromISR(shellTaskHandle);
//}
