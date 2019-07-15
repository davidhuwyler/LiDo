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

void UI_Init(void);
void UI_StopShellIndicator(void);
void UI_LEDpulse(UI_LEDs_t color);
void UI_ButtonPressed_ISR(void);


#endif /* SOURCES_UI_H_ */
