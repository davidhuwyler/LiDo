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
 *
 *      Debouncin internet resource:
 *      http://www.ganssle.com/debouncing.htm
 */
#include "UI.h"
#include "FRTOS1.h"
#include "CLS1.h"
#include "Application.h"
#include "AppDataFile.h"
#include "LED_R.h"
#include "Shell.h"


static xTimerHandle uiButtonMultiPressTimer;
static xTimerHandle uiButtonDebounceTimer;

static xTimerHandle uiLEDtoggleTimer;
static xTimerHandle uiLEDmodeIndicatorTimer;

static bool uiInitDone = FALSE;
static bool ongoingButtonPress = FALSE;
static uint8_t buttonCnt = 0;
static uint8_t localNofBtnConfirmBlinks = 0;


static void UI_StartBtnConfirmBlinker(uint8_t nofBtnConfirmBlinks)
{
	LED_R_On();
	localNofBtnConfirmBlinks = nofBtnConfirmBlinks;
	if(xTimerChangePeriod(uiLEDmodeIndicatorTimer,400,0) != pdPASS){for(;;);}
	if(xTimerReset(uiLEDmodeIndicatorTimer, 0)!=pdPASS) { for(;;);}
}

static void UI_Button_1pressDetected(void)
{
	UI_StartBtnConfirmBlinker(1);
	APP_setMarkerInLog();
}

static void UI_Button_2pressDetected(void)
{
	UI_StartBtnConfirmBlinker(2);
	APP_toggleEnableSampling();
}

static void UI_Button_3pressDetected(void)
{

}

static void UI_Button_4pressDetected(void)
{
	UI_StartBtnConfirmBlinker(4);
	SHELL_requestDisabling();
}

static void UI_Button_5pressDetected(void)
{
	UI_StartBtnConfirmBlinker(5);
	APP_requestForSoftwareReset();
}


static void UI_ButtonCounter(void)
{
	static TickType_t lastButtonPressTimeStamp;
	if(!ongoingButtonPress)
	{
		 ongoingButtonPress = TRUE;
		 buttonCnt++;

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

	if(USER_BUTTON_PRESSED)
	{
		if (xTimerReset(uiButtonDebounceTimer,0)!=pdPASS)
		{
		   for(;;); /* failure?!? */
		}
	}
	else
	{
		if (xTimerReset(uiButtonMultiPressTimer, 0)!=pdPASS)
		{
		   for(;;); /* failure?!? */
		}
		ongoingButtonPress = FALSE;
	}

}

static void vTimerCallback_LED_ShellInicator(xTimerHandle pxTimer)
{
	LED_R_Neg();
}

static void vTimerCallback_LED_ModeIndicator(xTimerHandle pxTimer)
{
	uint16_t timerDelayMS = 0;
	if(LED_R_Get())
	{
		localNofBtnConfirmBlinks--;
		LED_R_Off();
		timerDelayMS = 200;
	}
	else
	{
		LED_R_On();
		timerDelayMS = 400;
	}

	if(localNofBtnConfirmBlinks)
	{
		if(xTimerReset(uiLEDmodeIndicatorTimer, 0)!=pdPASS) { for(;;);}
	}
}

void UI_StopShellIndicator(void)
{
	 if (xTimerDelete(uiLEDtoggleTimer, 0)!=pdPASS)
	 {
	    for(;;); /* failure?!? */
	 }
	 LED_R_Off();
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

	CS1_CriticalVariable();
	CS1_EnterCritical();
	uiInitDone = TRUE;
	CS1_ExitCritical();
}

void UI_ButtonPressed_ISR(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	if(uiInitDone)
	{
		CS1_ExitCritical();
		UI_ButtonCounter();
	}
	else
	{
		CS1_ExitCritical();
	}
}




