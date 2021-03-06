/*
 * AppDataFile.c
 *
 *  Created on: Mar 9, 2019
 *      Author: dave
 */

#include "AppDataFile.h"
#include "minIni/minIni.h"
#include "UTIL1.h"

#define APPDATA_FILENAME "./appData.ini"
#define APPDATA_SECTION "AppData"

#define DEFAULT_BOOL 0
#define DEFAULT_INT 1000
#define DEFAULT_STRING "-"

const char *APPDATA_KEYS_AND_DEV_VALUES[APPDATA_NOF_KEYS][2] =
{
		{"LIDO_NAME","Lido01"},			//Name
		{"LIDO_ID","00001"},			//ID
		{"LIDO_VERSION","V0.1"},		//SoftwareVersion
		{"SAMPLE_INTERVALL","1"},		//Sampleintervall [s]
		{"SAMPLE_ENABLE","0"},			//Sample enable defines if the LiDo is sampling (1 = sampling)
		{"LIGHTSENS_GAIN","3"},			//Gain of the LightSensor (0=1x; 1=3.7x; 2=16x; 3=64x;)
		{"LIGHTSENS_INTT","240"},		//LightSensor IntegrationTime  (256 - value) * 2.8ms
		{"LIGHTSENS_WTIM","240"}		//LightSensor Time between Conversions: (256 - value) * 2.8ms
};

static bool localSamplingEnabled = FALSE;
static uint8_t localSampleIntervall = 1;

static void AppDataFile_UpdateRAMvariables()
{
	uint8_t buf[25];
	const unsigned char *p;

	//Update local localSampleIntervall
	p = buf;
	AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[3][0], (uint8_t*)p ,25);
	UTIL1_ScanDecimal8uNumber(&p, &localSampleIntervall);

	//Update local localSamplingEnabled
	p = buf;
	AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0], (uint8_t*)p ,25);
	UTIL1_ScanDecimal8uNumber(&p,(uint8_t*)&localSamplingEnabled);
}

uint8_t AppDataFile_Init(void)
{
	AppDataFile_UpdateRAMvariables();
}

uint8_t AppDataFile_GetStringValue(const uint8_t* key, uint8_t* valueBuf ,size_t bufSize)
{
		if(ini_gets(APPDATA_SECTION, key, DEFAULT_STRING, valueBuf, bufSize, APPDATA_FILENAME) == 0)
		{
			return ERR_FAILED;
		}
		return ERR_OK;
}

uint8_t AppDataFile_GetSampleIntervall(uint8_t* sampleIntervall)
{
	if(localSampleIntervall<1 || localSampleIntervall>100)
	{
		*sampleIntervall = 1;
		return ERR_FAILED;
	}
	else
	{
		*sampleIntervall = localSampleIntervall;
		return ERR_OK;
	}
}

uint8_t AppDataFile_SetSampleIntervall(uint8_t sampleIntervall)
{
	localSampleIntervall = sampleIntervall;
}

bool AppDataFile_GetSamplingEnabled(void)
{
	return localSamplingEnabled;
}

void AppDataFile_SetSamplingEnables(bool samplingEnabled)
{
	localSamplingEnabled = samplingEnabled;
}

uint8_t AppDataFile_SetStringValue(const uint8_t* key, const uint8_t* value)
{
		if(ini_puts(APPDATA_SECTION, key, value, APPDATA_FILENAME) == 0)
		{
			return ERR_FAILED;
		}
		AppDataFile_UpdateRAMvariables();
		return ERR_OK;
}

uint8_t AppDataFile_CreateFile(void)
{
	for(int i = 0 ; i < APPDATA_NOF_KEYS ; i++)
	{
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[i][0],APPDATA_KEYS_AND_DEV_VALUES[i][1]);
	}
	return ERR_OK;
}

static uint8_t AppDataFile_PrintList(CLS1_ConstStdIOType *io)
{
	uint8_t lineBuffer[50];
	uint8_t valueBuffer[25];
	for(int i = 0 ; i < APPDATA_NOF_KEYS ; i++)
	{
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[i][0],valueBuffer,25);
		UTIL1_strcpy(lineBuffer,50,APPDATA_KEYS_AND_DEV_VALUES[i][0]);
		UTIL1_strcat(lineBuffer,50," = ");
		UTIL1_strcat(lineBuffer,50,valueBuffer);
		UTIL1_strcat(lineBuffer,50,"\r\n");
		CLS1_SendStr(lineBuffer,io->stdErr);
	}
	return ERR_OK;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"AppData", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t AppData_ParseCommand(const uint8_t *cmd, bool *handled, const CLS1_StdIOType *io)
{
  uint8_t res = ERR_OK;
  const uint8_t *p;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "AppData help")==0)
  {
    CLS1_SendHelpStr((unsigned char*)"AppData", (const unsigned char*)"Group of AppData File commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  create", (const unsigned char*)"Creates the appData.ini file\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  print", (const unsigned char*)"Prints the appData.ini file\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  }
  else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LightSens status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  else if (UTIL1_strcmp((char*)cmd, "AppData create")==0)
  {
    *handled = TRUE;
    return AppDataFile_CreateFile();
  }
  else if (UTIL1_strcmp((char*)cmd, "AppData print")==0)
  {
    *handled = TRUE;
    return AppDataFile_PrintList(io);
  }
  return res;
}
