/*
 * ErrorLogFile.c
 *
 *  Created on: 25.03.2019
 *      Author: dhuwiler
 */

#include "ErrorLogFile.h"
#include "TmDt1.h"
#include "FRTOS1.h"
#include "UTIL1.h"
#include "minIni/minIni.h"

#define ERRORLOG_FILENAME "./errorLog.ini"
#define ERRORLOG_SECTION "ErrorLog"

#define DEFAULT_BOOL 0
#define DEFAULT_INT 1000
#define DEFAULT_STRING "-"

uint8_t ErrorLogFile_Init(void)
{

}

uint8_t ErrorLogFile_LogError(uint16 SDEP_error_alert_cmdId, uint8_t* message)
{
	SemaphoreHandle_t fileSema;
	TIMEREC time;
	DATEREC date;
	uint8_t keyBuf[50];

	UTIL1_Num16uToStr(keyBuf,50,SDEP_error_alert_cmdId);
	UTIL1_strcat(keyBuf,50," ");
	TmDt1_GetInternalRTCTimeDate(&time,&date);
	TmDt1_AddDateString(keyBuf,50,&date,"dd.mm.yyyy");
	UTIL1_strcat(keyBuf,50," ");
	TmDt1_AddTimeString(keyBuf,50,&time,"hh:mm.ss,cc");

	FS_GetFileAccessSemaphore(&fileSema);
	if(xSemaphoreTake(fileSema,pdMS_TO_TICKS(500)))
	{
		if(ini_puts(ERRORLOG_SECTION, keyBuf, message, ERRORLOG_FILENAME) == 0)
		{
			xSemaphoreGive(fileSema);
			return ERR_FAILED;
		}
		xSemaphoreGive(fileSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}
}

static uint8_t ErrorLogFile_PrintList(CLS1_ConstStdIOType *io)
{
	SemaphoreHandle_t fileSema;
	uint8_t lineBuffer[100];
	uint8_t valueBuffer[50];
	uint8_t keyBuffer[50];

	FS_GetFileAccessSemaphore(&fileSema);
	if(xSemaphoreTake(fileSema,pdMS_TO_TICKS(500)))
	{
		int i = 0;
		while(ini_getkey(ERRORLOG_SECTION, i, keyBuffer, 50, ERRORLOG_FILENAME) > 0)
		{

			i++;

			if(ini_gets(ERRORLOG_SECTION, keyBuffer, DEFAULT_STRING, valueBuffer, 50, ERRORLOG_FILENAME) == 0)
			{
				xSemaphoreGive(fileSema);
				return ERR_FAILED;
			}

			UTIL1_strcpy(lineBuffer,100,keyBuffer);
			UTIL1_strcat(lineBuffer,100," Message: ");
			UTIL1_strcat(lineBuffer,100,valueBuffer);
			UTIL1_strcat(lineBuffer,100,"\r\n");
			CLS1_SendStr(lineBuffer,io->stdErr);
		}

		xSemaphoreGive(fileSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}
}


static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"ErrorLogFile", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t ErrorLogFile_ParseCommand(const uint8_t *cmd, bool *handled, const CLS1_StdIOType *io)
{
	  uint8_t res = ERR_OK;
	  const uint8_t *p;

	  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "ErrorLogFile help")==0)
	  {
	    CLS1_SendHelpStr((unsigned char*)"ErrorLogFile", (const unsigned char*)"Group of AppData File commands\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  print", (const unsigned char*)"Prints the appData.ini file\r\n", io->stdOut);
	    *handled = TRUE;
	    return ERR_OK;
	  }
	  else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LightSens status")==0)) {
	    *handled = TRUE;
	    return PrintStatus(io);
	  }

	  else if (UTIL1_strcmp((char*)cmd, "ErrorLogFile print")==0)
	  {
	    *handled = TRUE;
	    return ErrorLogFile_PrintList(io);
	  }
	  return res;
}


