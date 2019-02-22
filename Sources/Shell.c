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

  for(;;)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  (void)CLS1_ReadAndParseWithCommandTable(CLS1_DefaultShellBuffer, sizeof(CLS1_DefaultShellBuffer), CLS1_GetStdio(), CmdParserTable);
	  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(100));
  } /* for */
}

void SHELL_Init(void) {
  CLS1_DefaultShellBuffer[0] = '\0';
  if (xTaskCreate(SHELL_task, "Shell", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
  {
	  for(;;){} /* error! probably out of memory */
  }
}


