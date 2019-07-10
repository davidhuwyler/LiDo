/*
 * UI.h
 *
 *  Created on: Mar 15, 2019
 *      Author: dave
 */

#ifndef SOURCES_UI_H_
#define SOURCES_UI_H_

typedef enum
{
	LED_R,
	LED_G,
	LED_B,
	LED_Y,
	LED_V, /* sample indication */
	LED_C,
	LED_W
} UI_LEDs_t;

#define UI_BUTTON_TIMEOUT_BETWEEN_TWO_PRESSES_MS 1000
#define UI_BUTTON_DEBOUNCE_INTERVALL_MS 20

#define UI_LED_SHELL_INDICATOR_TOGGLE_DELAY_MS 200
#define UI_LED_MODE_INDICATOR_DURATION_MS 300
#define UI_LED_PULSE_INDICATOR_DURATION_MS 1

void UI_Init(void);
void UI_StopShellIndicator(void);
void UI_LEDpulse(UI_LEDs_t color);
void UI_ButtonPressed_ISR(void);


#endif /* SOURCES_UI_H_ */
