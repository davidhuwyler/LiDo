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

static xTimerHandle uiButtonMultiPressTimer;
static xTimerHandle uiButtonDebounceTimer;

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


}

static void UI_Button_5pressDetected(void)
{
	//Softrese: Enable shell for 20s
	APP_requestForSoftwareReset();
}


void UI_ButtonCounter(void)
{
	static TickType_t lastButtonPressTimeStamp;
	if(!buttonDebouncingStarted)
	{
		 buttonDebouncingStarted = true;
		 if (xTimerStartFromISR(uiButtonDebounceTimer, 0)!=pdPASS)
		 { /* start timer to turn off LCD after 5 seconds */
		    for(;;); /* failure?!? */
		 }

		 if (xTimerResetFromISR(uiButtonMultiPressTimer, 0)!=pdPASS)
		 { /* start timer to turn off LCD after 5 seconds */
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
	if(ExtInt_UI_BTN_GetVal() == false) // --> Button is still Pressed
	{
		buttonCnt++;
	}
	buttonDebouncingStarted = false;
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


}





