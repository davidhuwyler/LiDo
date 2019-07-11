/*
 * PowerManagement.c
 *
 *  Created on: Apr 25, 2019
 *      Author: dave
 */

#include "Platform.h"
#include "PowerManagement.h"
#include "PIN_POWER_ON.h"
#include "PIN_PWR_CHARGE_STATE.h"
#if PL_CONFIG_HAS_BATT_ADC
  #include "PIN_EN_U_MEAS.h"
  #include "AI_PWR_0_5x_U_BAT.h"
#endif
#if PL_CONFIG_HAS_GAUGE_SENSOR
  #include "McuLC709203F.h"
#endif
#include "FRTOS1.h"
#include "SDEP.h"
#include "UI.h"
#include "LED_B.h"

#define POWER_MANAGEMENT_LIPO_WARNING 34753 // = 3.5V --> ADCval = U / 2 * ( 65535 / 3.3V ) --> approx 10% Capacity
#define POWER_MANAGEMENT_LIPO_CUTOFF 29789  // = 3.0V --> ADCval = U / 2 * ( 65535 / 3.3V )
#define POWER_MANAGEMENT_LIPO_WARNING_HYS 36739 // = 3.7V --> ADCval = U / 2 * ( 65535 / 3.3V )

#define POWER_MANAGEMENT_LIPO_OFFSET 650

static TaskHandle_t powerManagementTaskHandle;
static bool waringLogged = FALSE;

bool PowerManagement_IsCharging(void) {
  return PIN_PWR_CHARGE_STATE_GetVal()==TRUE; /* pin is HIGH/true if charging */
}

static void PowerManagement_task(void *param) {
  (void)param;
#if PL_CONFIG_HAS_BATT_ADC
  uint16_t adcValue = 0x0000;
  uint16_t oldAdcValue =  36000; // = 3.7V
#endif

  for(;;)
  {
#if PL_CONFIG_HAS_BATT_ADC
    PIN_EN_U_MEAS_SetVal();
  	WAIT1_Waitus(10);
    AI_PWR_0_5x_U_BAT_Measure(FALSE);
    WAIT1_Waitus(20);
    AI_PWR_0_5x_U_BAT_GetValue16(&adcValue);
    PIN_EN_U_MEAS_ClrVal();
	  adcValue = adcValue + POWER_MANAGEMENT_LIPO_OFFSET;

	  if(PIN_CHARGE_STATE_GetVal()) {
		  UI_LEDpulse(LED_G);
	  }

    if(oldAdcValue < POWER_MANAGEMENT_LIPO_WARNING && adcValue < POWER_MANAGEMENT_LIPO_WARNING)
    {
        UI_LEDpulse(LED_R);

        if(!waringLogged)
        {
            SDEP_InitiateNewAlertWithMessage(SDEP_ALERT_LOW_BATTERY,"Battery is low...");
            waringLogged = TRUE;
        }
    }

    if(oldAdcValue > POWER_MANAGEMENT_LIPO_WARNING_HYS && adcValue > POWER_MANAGEMENT_LIPO_WARNING_HYS)
    {
        waringLogged = FALSE;
    }

	  if(oldAdcValue < POWER_MANAGEMENT_LIPO_CUTOFF && adcValue < POWER_MANAGEMENT_LIPO_CUTOFF)
	  {
		  PIN_POWER_ON_ClrVal(); /* emergency: cut off power */
	  }
	  oldAdcValue = adcValue;
#else
    if(PowerManagement_IsCharging()) {
      LED_B_Off(); /* turn off blue led as this one might indicate a shell connection */
      UI_LEDpulse(LED_G);
    }
#endif
	  vTaskSuspend(powerManagementTaskHandle);
  }
}

void PowerManagement_ResumeTaskIfNeeded(void) {
  static TickType_t xLastWakeTime = 0;

  if (xTaskGetTickCount()-xLastWakeTime > 10000 && powerManagementTaskHandle!=NULL) {
	  xLastWakeTime = xTaskGetTickCount();
	  vTaskResume(powerManagementTaskHandle);
  }
}

//Return the Battery Voltage in mV
uint16_t PowerManagement_getBatteryVoltage(void) {
#if PL_CONFIG_HAS_BATT_ADC
	uint16_t adcValue;
	PIN_EN_U_MEAS_SetVal();
	WAIT1_Waitus(10);
	AI_PWR_0_5x_U_BAT_Measure(TRUE);
	PIN_EN_U_MEAS_ClrVal();
	AI_PWR_0_5x_U_BAT_GetValue16(&adcValue);
	return (adcValue+POWER_MANAGEMENT_LIPO_OFFSET)/10;
#elif PL_CONFIG_HAS_GAUGE_SENSOR
	uint8_t res;
	uint16_t voltage;

	res = McuLC_GetVoltage(&voltage);
	if (res==ERR_OK) {
	  return voltage;
	}
	return 0; /* error case */
#else
	return 0;
#endif
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  CLS1_SendStatusStr((unsigned char*)"power", (const unsigned char*)"\r\n", io->stdOut);
  CLS1_SendStatusStr((unsigned char*)"  charging", PowerManagement_IsCharging()?(unsigned char*)"yes\r\n":(unsigned char*)"no\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t PowerManagement_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint8_t res = ERR_OK;
  const uint8_t *p;
  int32_t tmp;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "power help")==0) {
    CLS1_SendHelpStr((unsigned char*)"power", (const unsigned char*)"Group of power management commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  } else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "power status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  return res;
}

void PowerManagement_init(void) {
  if (xTaskCreate(PowerManagement_task, "PowerManagement", 700/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, &powerManagementTaskHandle) != pdPASS)
  {
    for(;;){} /* error! probably out of memory */
  }
}

