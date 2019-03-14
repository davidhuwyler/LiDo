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
//#include "littleFS/lfs.h"
#include "CLS1.h"

#define APPDATA_NOF_KEYS 5
const char *APPDATA_KEYS_AND_DEV_VALUES[APPDATA_NOF_KEYS][2] =
{
		{"LIDO_NAME","Lido01"},			//Name
		{"LIDO_ID","00001"},			//ID
		{"LIDO_VERSION","V0.1"},		//SoftwareVersion
		{"SAMPLE_INTERVALL","1"},		//Sampleintervall [s]
		{"SAMPLE_ENABLE","1"}			//Sample enable defines if the LiDo is sampling (1 = sampling)
};

uint8_t AppDataFile_Init(void);
uint8_t AppDataFile_GetSampleIntervall(uint8_t* sampleIntervall);
bool AppDataFile_GetSamplingEnabled(void);
uint8_t AppDataFile_GetStringValue(const uint8_t* key, uint8_t* valueBuf ,size_t bufSize);
uint8_t AppDataFile_SetStringValue(const uint8_t* key, const uint8_t* value);
uint8_t AppData_ParseCommand(const uint8_t *cmd, bool *handled, const CLS1_StdIOType *io);

#endif /* SOURCES_APPDATAFILE_H_ */
