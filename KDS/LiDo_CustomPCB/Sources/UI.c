/*
 * UI.c
 *
 *  Created on: Mar 15, 2019
 *      Author: dave
 *
 *      UserInterface of the LiDo
 *
 *      - User Button
 *      - LEDs
 *
 *      Debouncing internet resource:
 *      http://www.ganssle.com/debouncing.htm
 */
#include "Platform.h"
#include "UI.h"
#include "Application.h"
#include "AppDataFile.h"
#if (PL_BOARD_REV==20 || PL_BOARD_REV==21)
  //#include "PTC.h" /* uses PTC1 for user button */
#elif (PL_BOARD_REV==22)
  //#include "PTD.h" /* uses PTD0 for user button */
#else
  #error "unknown board"
#endif
#include "PORT_PDD.h"
#include "FRTOS1.h"
#include "CLS1.h"
#include "LED_R.h"
#include "LED_G.h"
#include "LED_B.h"
#include "Shell.h"

#define UI_BUTTON_LED_CONFIRMATION   (0)  /* if button press shall be confirmed by blue LED */
#define UI_BUTTON_TIMEOUT_BETWEEN_TWO_PRESSES_MS (1000)  /* used to count duration of button press */
#define UI_BUTTON_DEBOUNCE_INTERVALL_MS          (20)

/* button press counter confirmation LED blinking */
#define UI_LED_MODE_INDICATOR_DURATION_ON_MS     (10) /* time the LED is on */
#define UI_LED_MODE_INDICATOR_DURATION_OFF_MS    (990) /* time the LED is off */
static uint8_t localNofBtnConfirmBlinks = 0; /* nof confirmation button blink cylces. Is twice the number of blinks because of on and off phases */

#define UI_LED_PULSE_INDICATOR_DURATION_MS       (1)

static TimerHandle_t uiButtonMultiPressTimer;
static TimerHandle_t uiButtonDebounceTimer;
static TimerHandle_t uiLEDmodeIndicatorTimer;
static TimerHandle_t uiLEDpulseIndicatorTimer;

static bool uiInitDone = FALSE;  /* if UI (this module) has been initialized */
static bool ongoingButtonPress = FALSE; /* button is kept pressed */
static uint8_t buttonCnt = 0;

static void vTimerCallback_LED_ModeIndicator(TimerHandle_t pxTimer) {
  uint16_t timerDelayMS = 0;

  localNofBtnConfirmBlinks--;
  if((localNofBtnConfirmBlinks%2)==0) { /* even number: LED is on */
    LED_R_Off();
    LED_B_Off();
    LED_G_Off();
    timerDelayMS = UI_LED_MODE_INDICATOR_DURATION_OFF_MS;
  } else {
    LED_R_Off();
    LED_B_Off();
    LED_G_On();
    timerDelayMS = UI_LED_MODE_INDICATOR_DURATION_ON_MS;
  }
  if(localNofBtnConfirmBlinks>0) { /* still blinkys to go on ... */
    if (xTimerChangePeriod(uiLEDmodeIndicatorTimer, timerDelayMS, 0) != pdPASS){
      APP_FatalError(__FILE__, __LINE__);
    }
    if (xTimerReset(uiLEDmodeIndicatorTimer, 0)!=pdPASS) { /* start timer */
      APP_FatalError(__FILE__, __LINE__);
    }
  } else { /* stop timer */
    if (xTimerStop(uiLEDmodeIndicatorTimer, 0)!=pdPASS) {
      APP_FatalError(__FILE__, __LINE__);
    }
  }
}

static void UI_StartBtnConfirmBlinker(uint8_t nofBtnConfirmBlinks) {
  localNofBtnConfirmBlinks = (nofBtnConfirmBlinks*2)-1; /* twice because of on and then off */
  /* set to initial on period */
  LED_R_Off();
  LED_B_Off();
  LED_G_On();
  if (xTimerChangePeriod(uiLEDmodeIndicatorTimer, UI_LED_MODE_INDICATOR_DURATION_ON_MS, 0) != pdPASS){
    APP_FatalError(__FILE__, __LINE__);
  }
  if (xTimerReset(uiLEDmodeIndicatorTimer, 0)!=pdPASS) { /* start timer */
    APP_FatalError(__FILE__, __LINE__);
  }
}

static void UI_Button_1pressDetected(void) {
  if (AppDataFile_GetSamplingEnabled()) {
    UI_StartBtnConfirmBlinker(1);
    APP_setMarkerInLog();
  }
}

static void UI_Button_2pressDetected(void) {
	UI_StartBtnConfirmBlinker(2);
	APP_toggleEnableSampling();
}

static void UI_Button_3pressDetected(void) {
}

static void UI_Button_4pressDetected(void) {
#if PL_CONFIG_HAS_SHELL
	UI_StartBtnConfirmBlinker(4);
	SHELL_requestDisabling();
#endif
}

static void UI_Button_5pressDetected(void) {
	UI_StartBtnConfirmBlinker(5);
	APP_requestForSoftwareReset();
}

static void UI_Button_6pressDetected(void) {
  UI_StartBtnConfirmBlinker(6);
  APP_requestForPowerOff();
}

static void UI_ButtonCounter(void) {
	if(!ongoingButtonPress) {
    ongoingButtonPress = TRUE;
    buttonCnt++;
#if UI_BUTTON_LED_CONFIRMATION
    LED_B_On(); /* will be turned off uiButtonDebounceTimer */
#endif
    if (xTimerStartFromISR(uiButtonDebounceTimer, 0)!=pdPASS) {
     APP_FatalError(__FILE__, __LINE__);
    }
    if (xTimerResetFromISR(uiButtonMultiPressTimer, 0)!=pdPASS) {
     APP_FatalError(__FILE__, __LINE__);
    }
	}
}

static void vTimerCallback_ButtonMultiPressTimer(TimerHandle_t pxTimer) {
	switch(buttonCnt) {
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
    case 6:
      UI_Button_6pressDetected();
      break;
    default:
      break;
	} /* switch */
	buttonCnt = 0;
}

static void vTimerCallback_ButtonDebounceTimer(TimerHandle_t pxTimer) {
	if (APP_UserButtonPressed()) { /* button still pressed */
		if (xTimerReset(uiButtonDebounceTimer,0)!=pdPASS) {
		  APP_FatalError(__FILE__, __LINE__);
		}
	} else { /* button released */
#if UI_BUTTON_LED_CONFIRMATION
	  LED_B_Off(); /* turn LED off which has been turned on at button press time */
#endif
		if (xTimerReset(uiButtonMultiPressTimer, 0)!=pdPASS) {
		  APP_FatalError(__FILE__, __LINE__);
		}
		ongoingButtonPress = FALSE;
	}
}

static void vTimerCallback_LEDpulse(TimerHandle_t pxTimer) {
  /* end of LED pulse: turn off all LEDs */
	LED_R_Off();
	LED_G_Off();
	LED_B_Off();
}

void UI_LEDpulse(UI_LEDs_t color) {
	switch(color) {
	case LED_R:
		LED_R_On();
		break;

	case LED_G:
		LED_G_On();
		break;

	case LED_B:
		LED_B_On();
		break;

	case LED_Y:
		LED_R_On();
		LED_G_On();
		break;

	case LED_C:
		LED_B_On();
		LED_G_On();
		break;

	case LED_V:
		LED_B_On();
		LED_R_On();
		break;

	case LED_W:
		LED_B_On();
		LED_G_On();
		LED_R_On();
		break;
	} /* switch */
	if (uiLEDpulseIndicatorTimer!=NULL && xTimerReset(uiLEDpulseIndicatorTimer, 0)!=pdPASS) {
	  APP_FatalError(__FILE__, __LINE__);
	}
}

void UI_Init(void) {
	uiButtonMultiPressTimer = xTimerCreate( "tmrUiBtn", /* name */
										 pdMS_TO_TICKS(UI_BUTTON_TIMEOUT_BETWEEN_TWO_PRESSES_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)0, /* timer ID */
										 vTimerCallback_ButtonMultiPressTimer); /* callback */
	if (uiButtonMultiPressTimer==NULL) {
	  APP_FatalError(__FILE__, __LINE__);
	}
	uiButtonDebounceTimer = xTimerCreate( "tmrUiBtnDeb", /* name */
										 pdMS_TO_TICKS(UI_BUTTON_DEBOUNCE_INTERVALL_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)1, /* timer ID */
										 vTimerCallback_ButtonDebounceTimer); /* callback */
	if (uiButtonDebounceTimer==NULL) {
	  APP_FatalError(__FILE__, __LINE__);
	}
	uiLEDmodeIndicatorTimer = xTimerCreate( "tmrUiLEDmodeInd", /* name */
										 pdMS_TO_TICKS(UI_LED_MODE_INDICATOR_DURATION_ON_MS+UI_LED_MODE_INDICATOR_DURATION_OFF_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)3, /* timer ID */
										 vTimerCallback_LED_ModeIndicator); /* callback */
	if (uiLEDmodeIndicatorTimer==NULL) {
	  APP_FatalError(__FILE__, __LINE__);
	}
	uiLEDpulseIndicatorTimer = xTimerCreate( "tmrUiLEDpulse", /* name */
										 pdMS_TO_TICKS(UI_LED_PULSE_INDICATOR_DURATION_MS), /* period/time */
										 pdFALSE, /* auto reload */
										 (void*)4, /* timer ID */
										 vTimerCallback_LEDpulse); /* callback */
	if (uiLEDpulseIndicatorTimer==NULL) {
	  APP_FatalError(__FILE__, __LINE__);
	}
	uiInitDone = TRUE;
}

void UI_ButtonPressed_ISR(void) { /* wakeup and interrupt by user button press */
#if (PL_BOARD_REV==20 || PL_BOARD_REV==21) /* uses PTC1 for user button */
  //PORT_PDD_ClearInterruptFlags(PTC_PORT_DEVICE,1U<<1);
#elif (PL_BOARD_REV==22) /* uses PTD0 for user button */
  //PORT_PDD_ClearInterruptFlags(PTD_PORT_DEVICE,1U<<0);
#else
  #error "unknown board"
#endif
	if(uiInitDone) {
		UI_ButtonCounter();
	}
}
