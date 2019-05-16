/*
 * AppDataFile.h
 *
 *  Created on: Mar 9, 2019
 *      Author: dave
 */

#ifndef SOURCES_APPDATAFILE_H_
#define SOURCES_APPDATAFILE_H_

#include <stdint.h>
#include "FileSystem.h"
#include "CLS1.h"

#define APPDATA_NOF_KEYS 10
extern const char *APPDATA_KEYS_AND_DEV_VALUES[APPDATA_NOF_KEYS][2];

uint8_t AppDataFile_Init(void);
uint8_t AppDataFile_CreateFile(void);

uint8_t AppDataFile_GetSampleIntervall(uint8_t* sampleIntervall);
bool AppDataFile_GetSamplingEnabled(void);
bool AppDataFile_GetAutoGainEnabled(void);
bool AppDataFile_GetSampleAutoOff(void);

uint8_t AppDataFile_GetStringValue(const uint8_t* key, uint8_t* valueBuf ,size_t bufSize);
uint8_t AppDataFile_SetStringValue(const uint8_t* key, const uint8_t* value);
uint8_t AppData_ParseCommand(const uint8_t *cmd, bool *handled, const CLS1_StdIOType *io);

#endif /* SOURCES_APPDATAFILE_H_ */
