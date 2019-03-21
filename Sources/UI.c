/*
 * UI.c
 *
 *  Created on: Mar 15, 2019
 *      Author: dave
 *
 *      UserInterface of the LiDo
 *
 *      - User Button PTC11
 *      - LEDs
 */
#include "UI.h"
#include "FRTOS1.h"
#include "CLS1.h"
#include "ExtInt_UI_BTN.h"
#include "Application.h"
#include "AppDataFile.h"
#include "LED1.h"
#include "Shell.h"


static xTimerHandle uiButtonMultiPressTimer;
static xTimerHandle uiButtonDebounceTimer;

static xTimerHandle uiLEDtoggleTimer;
static xTimerHandle uiLEDmodeIndicatorTimer;

static bool buttonDebouncingStarted = FALSE;
static uint8_t buttonCnt = 0;


static void UI_Button_1pressDetected(void)
{
	APP_setMarkerInLog();
}

static void UI_Button_2pressDetected(void)
{
	 APP_toggleEnableSampling();
}

static void UI_Button_3pressDetected(void)
{

}

static void UI_Button_4pressDetected(void)
{
	LED1_On();
	xTimerChangePeriod(uiLEDmodeIndicatorTimer,pdMS_TO_TICKS(UI_LED_MODE_INDICATOR_DURATION_MS),0);
	xTimerStartFromISR(uiLEDmodeIndicatorTimer, 0);
	SHELL_requestDisabling();
}

static void UI_Button_5pressDetected(void)
{
	LED1_On();
	xTimerChangePeriod(uiLEDmodeIndicatorTimer,pdMS_TO_TICKS(UI_LED_MODE_INDICATOR_DURATION_MS*2),0);
	xTimerStartFromISR(uiLEDmodeIndicatorTimer, 0);
	APP_requestForSoftwareReset();
}


void UI_ButtonCounter(void)
{
	static TickType_t lastButtonPressTimeStamp;
	if(!buttonDebouncingStarted)
	{
		 buttonDebouncingStarted = TRUE;
		 if (xTimerStartFromISR(uiButtonDebounceTimer, 0)!=pdPASS)
		 {
		    for(;;); /* failure?!? */
		 }

		 if (xTimerResetFromISR(uiButtonMultiPressTimer, 0)!=pdPASS)
		 {
		    for(;;); /* failure?!? */
		 }
	}
}

static void vTimerCallback_ButtonMultiPressTimer(xTimerHandle pxTimer)
{
	switch(buttonCnt)
	{
	case 0:		//Should not happen...
		break;
	case 1:
		UI_Button_1pressDetected();
		break;
	case 2:
		UI_Button_2pressDetected();
		break;
	case 3:
		UI_Button_3pressDetected();
		break;
	case 4:
		UI_Button_4pressDetected();
		break;
	case 5:
		UI_Button_5pressDetected();
		break;
	default:
		break;
	}
	buttonCnt = 0;
}

static void vTimerCallback_ButtonDebounceTimer(xTimerHandle pxTimer)
{
	if(ExtInt_UI_BTN_GetVal() == FALSE) // --> Button is still Pressed
	{
		LED1_Neg();
	    buttonCnt++;
	}
	buttonDebouncingStarted = FALSE;
}

static void vTimerCallback_LED_ShellInicator(xTimerHandle pxTimer)
{
	LED1_Neg();
}

static void vTimerCallback_LED_ModeIndicator(xTimerHandle pxTimer)
{
	LED1_Off();
}

void UI_StopShellIndicator(void)
{
	 if (xTimerDelete(uiLEDtoggleTimer, 0)!=pdPASS)
	 {
	    for(;;); /* failure?!? */
	 }
	 LED1_Off();
}


void UI_Init(void)
{
	uiButtonMultiPressTimer = xTimerCreate( "tmrUiBtn", /* name */
										 pdMS_TO_TICKS(UI_BUTTON_TIMEOUT_BETWEEN_TWO_PRESSES_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)0, /* timer ID */
										 vTimerCallback_ButtonMultiPressTimer); /* callback */

	if (uiButtonMultiPressTimer==NULL) { for(;;); /* failure! */}

	uiButtonDebounceTimer = xTimerCreate( "tmrUiBtnDeb", /* name */
										 pdMS_TO_TICKS(UI_BUTTON_DEBOUNCE_INTERVALL_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)1, /* timer ID */
										 vTimerCallback_ButtonDebounceTimer); /* callback */

	if (uiButtonDebounceTimer==NULL) { for(;;); /* failure! */}

	uiLEDtoggleTimer = xTimerCreate( "tmrUiLEDtoggle", /* name */
										 pdMS_TO_TICKS(UI_LED_SHELL_INDICATOR_TOGGLE_DELAY_MS), /* period/time */
										 pdTRUE, /* auto reload */
										 (void*)2, /* timer ID */
										 vTimerCallback_LED_ShellInicator); /* callback */

	 if (xTimerStart(uiLEDtoggleTimer, 0)!=pdPASS)
	 {
	    for(;;); /* failure?!? */
	 }

	 uiLEDmodeIndicatorTimer = xTimerCreate( "tmrUiLEDmodeInd", /* name */
										 pdMS_TO_TICKS(UI_LED_MODE_INDICATOR_DURATION_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)3, /* timer ID */
										 vTimerCallback_LED_ModeIndicator); /* callback */

	if (uiButtonDebounceTimer==NULL) { for(;;); /* failure! */}


}





