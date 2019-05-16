/*
 * PowerManagement.c
 *
 *  Created on: Apr 25, 2019
 *      Author: dave
 */

#include "PowerManagement.h"
#include "PIN_POWER_ON.h"
#include "PIN_EN_U_MEAS.h"
#include "PIN_CHARGE_STATE.h"
#include "AI_PWR_0_5x_U_BAT.h"
#include "FRTOS1.h"
#include "SDEP.h"
#include "UI.h"

#define POWER_MANAGEMENT_LIPO_WARNING 34753 // = 3.5V --> ADCval = U / 2 * ( 65535 / 3.3V ) --> approx 10% Capacity
#define POWER_MANAGEMENT_LIPO_CUTOFF 29789  // = 3.0V --> ADCval = U / 2 * ( 65535 / 3.3V )
#define POWER_MANAGEMENT_LIPO_OFFSET 1250

static TaskHandle_t powerManagementTaskHandle;

static void PowerManagement_task(void *param) {
  (void)param;
  uint16_t adcValue = 0x0000;
  uint16_t oldAdcValue =  36000; // = 3.7V

  for(;;)
  {
      PIN_EN_U_MEAS_SetVal();
  	  WAIT1_Waitus(10);
      AI_PWR_0_5x_U_BAT_Measure(FALSE);
      WAIT1_Waitus(20);
      AI_PWR_0_5x_U_BAT_GetValue16(&adcValue);
      PIN_EN_U_MEAS_ClrVal();
	  adcValue = adcValue + POWER_MANAGEMENT_LIPO_OFFSET;

	  if(PIN_CHARGE_STATE_GetVal())
	  {
		  UI_LEDpulse(LED_G);
	  }

	  if(oldAdcValue < POWER_MANAGEMENT_LIPO_WARNING && adcValue < POWER_MANAGEMENT_LIPO_WARNING)
	  {
		  UI_LEDpulse(LED_R);
	  }

	  if(oldAdcValue < POWER_MANAGEMENT_LIPO_CUTOFF && adcValue < POWER_MANAGEMENT_LIPO_CUTOFF)
	  {
		  PIN_POWER_ON_ClrVal(); // PowerOff the device
	  }
	  oldAdcValue = adcValue;

	  vTaskSuspend(powerManagementTaskHandle);
	  //vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

void PowerManagement_ResumeTaskIfNeeded(void)
{
  static TickType_t xLastWakeTime = 0;
  if(xTaskGetTickCount()-xLastWakeTime> 10000 && powerManagementTaskHandle!=NULL)
  {
	  xLastWakeTime = xTaskGetTickCount();
	  vTaskResume( powerManagementTaskHandle );
  }
}

//Return the Battery Voltage in mV
uint16_t PowerManagement_getBatteryVoltage(void)
{
	uint16_t adcValue;
	PIN_EN_U_MEAS_SetVal();
	WAIT1_Waitus(10);
	AI_PWR_0_5x_U_BAT_Measure(TRUE);
	PIN_EN_U_MEAS_ClrVal();
	AI_PWR_0_5x_U_BAT_GetValue16(&adcValue);
	return (adcValue+POWER_MANAGEMENT_LIPO_OFFSET)/10;
}

void PowerManagement_init(void)
{
	  if (xTaskCreate(PowerManagement_task, "PowerManagement", 500/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, &powerManagementTaskHandle) != pdPASS)
	  {
		  for(;;){} /* error! probably out of memory */
	  }
}

