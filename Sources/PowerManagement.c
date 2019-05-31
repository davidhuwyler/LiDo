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
#define POWER_MANAGEMENT_LIPO_WARNING_HYS 36739 // = 3.7V --> ADCval = U / 2 * ( 65535 / 3.3V )

static bool waringLogged = FALSE;

static void PowerManagement_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  uint16_t adcValue,oldAdcValue;
  for(;;)
  {
#if PL_CONFIG_HAS_BAT_ACC
	  PIN_EN_U_MEAS_SetVal();
	  vTaskDelay(pdMS_TO_TICKS(1));
	  AI_PWR_0_5x_U_BAT_Measure(FALSE);
	  vTaskDelay(pdMS_TO_TICKS(1));
	  PIN_EN_U_MEAS_ClrVal();
	  AI_PWR_0_5x_U_BAT_GetValue16(&adcValue);

	  if(PIN_CHARGE_STATE_GetVal())
	  {
		  UI_LEDpulse(LED_G);
	  }

	  if(oldAdcValue < POWER_MANAGEMENT_LIPO_WARNING && adcValue < POWER_MANAGEMENT_LIPO_WARNING)
	  {
		  UI_LEDpulse(LED_R);

		  if(!waringLogged)
		  {
			  SDEP_InitiateNewAlert(SDEP_ALERT_LOW_BATTERY);
			  waringLogged = TRUE;
		  }
	  }

	  if(oldAdcValue > POWER_MANAGEMENT_LIPO_WARNING_HYS && adcValue > POWER_MANAGEMENT_LIPO_WARNING_HYS)
	  {
		  waringLogged = FALSE;
	  }

	  if(oldAdcValue < POWER_MANAGEMENT_LIPO_CUTOFF && adcValue < POWER_MANAGEMENT_LIPO_CUTOFF)
	  {
		  PIN_POWER_ON_ClrVal(); // PowerOff the device
	  }
	  oldAdcValue = adcValue;
#endif
	  vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

//Return the Battery Voltage in mV
uint16_t PowerManagement_getBatteryVoltage(void)
{
#if PL_CONFIG_HAS_BAT_ACC
	uint16_t adcValue;
	PIN_EN_U_MEAS_SetVal();
	WAIT1_Waitus(10);
	AI_PWR_0_5x_U_BAT_Measure(TRUE);
	PIN_EN_U_MEAS_ClrVal();
	AI_PWR_0_5x_U_BAT_GetValue16(&adcValue);
	return adcValue/10;
#else
	return 0;
#endif
}

void PowerManagement_init(void)
{
	  if (xTaskCreate(PowerManagement_task, "PowerManagement", 1000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
	  {
		  for(;;){} /* error! probably out of memory */
	  }
}

