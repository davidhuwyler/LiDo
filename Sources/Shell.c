/*
 * Shell.c
 *
 *  Created on: Feb 21, 2019
 *      Author: dave
 */
#include "Shell.h"
#include "CLS1.h"
#include "KIN1.h"
#include "LightSensor.h"
#include "AccelSensor.h"
#include "RTC.h"
#include "LowPower.h"
#include "CDC1.h"
#include "USB0.h"
#include "USB1.h"

static TaskHandle_t shellTaskHandle;
static TickType_t shellEnabledTimestamp;

static const CLS1_ParseCommandCallback CmdParserTable[] =
{
  CLS1_ParseCommand,
  KIN1_ParseCommand,
  LightSensor_ParseCommand,
  AccelSensor_ParseCommand,
  RTC_ParseCommand,
  NULL /* sentinel */
};

static void SHELL_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  shellEnabledTimestamp = xTaskGetTickCount();

  for(;;)
  {

	  xLastWakeTime = xTaskGetTickCount();
	  (void)CLS1_ReadAndParseWithCommandTable(CLS1_DefaultShellBuffer, sizeof(CLS1_DefaultShellBuffer), CLS1_GetStdio(), CmdParserTable);

	  if(xTaskGetTickCount() - shellEnabledTimestamp > pdMS_TO_TICKS(2000))
	  {
#if 0
		  LowPower_EnableStopMode();
		  vTaskSuspend(shellTaskHandle);¨
#elif 1
		  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(10));
#endif
	  }
	  else
	  {
		  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(10));
	  }
  } /* for */
}

void SHELL_Init(void) {
  CLS1_DefaultShellBuffer[0] = '\0';
  if (xTaskCreate(SHELL_task, "Shell", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &shellTaskHandle) != pdPASS)
  {
	  for(;;){} /* error! probably out of memory */
  }
}

void SHELL_EnableShellFor20s(void)
{
	LowPower_DisableStopMode();
	shellEnabledTimestamp = xTaskGetTickCount();
	xTaskResumeFromISR(shellTaskHandle);
	//vTaskResume(shellTaskHandle);
}

void SHELL_Disable(void)
{
	shellEnabledTimestamp = xTaskGetTickCount() - pdMS_TO_TICKS(20001);
}


