/*
 * LightAutoGain.h
 *
 *  Created on: 01.05.2019
 *      Author: dhuwiler
 */

#ifndef SOURCES_LIGHTAUTOGAIN_H_
#define SOURCES_LIGHTAUTOGAIN_H_

#include "Platform.h"
#include "Application.h"

uint8_t LiGain_Compute(liDoSample_t* lastSample, uint8_t* newIntTimeParam, uint8_t* newGainParam);

#endif /* SOURCES_LIGHTAUTOGAIN_H_ */
