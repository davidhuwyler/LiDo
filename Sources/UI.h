/*
 * UI.h
 *
 *  Created on: Mar 15, 2019
 *      Author: dave
 */

#ifndef SOURCES_UI_H_
#define SOURCES_UI_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define UI_BUTTON_TIMEOUT_BETWEEN_TWO_PRESSES_MS 500
#define UI_BUTTON_DEBOUNCE_INTERVALL_MS 8

#define UI_LED_SHELL_INDICATOR_TOGGLE_DELAY_MS 200
#define UI_LED_MODE_INDICATOR_DURATION_MS 300

//Called by the EXT Button interrupt
void UI_ButtonCounter(void);
void UI_Init(void);
void UI_StopShellIndicator(void);


#endif /* SOURCES_UI_H_ */
