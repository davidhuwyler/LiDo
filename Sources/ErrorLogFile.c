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
#include "FileSystem.h"

#define ERRORLOG_FILENAME "./errorLog.txt"
#define ERRORLOG_LINEBUFFER_SIZE 100

static lfs_file_t errorLogFile;
static uint8_t errorLogLineBuf[ERRORLOG_LINEBUFFER_SIZE];

uint8_t ErrorLogFile_Init(void)
{

}

uint8_t ErrorLogFile_LogError(uint16 SDEP_error_alert_cmdId, uint8_t* message)
{
	TIMEREC time;
	DATEREC date;

	UTIL1_Num16uToStr(errorLogLineBuf,ERRORLOG_LINEBUFFER_SIZE,SDEP_error_alert_cmdId);
	UTIL1_chcat(errorLogLineBuf,ERRORLOG_LINEBUFFER_SIZE, ' ');
	TmDt1_GetInternalRTCTimeDate(&time,&date);

	UTIL1_strcatNum16uFormatted(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, date.Day, '0', 2);
	UTIL1_chcat(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, '.');
	UTIL1_strcatNum16uFormatted(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, date.Month, '0', 2);
	UTIL1_chcat(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, '.');
	UTIL1_strcatNum16u(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, (uint16_t)date.Year);

	UTIL1_chcat(errorLogLineBuf,ERRORLOG_LINEBUFFER_SIZE, ' ');
	UTIL1_strcatNum16sFormatted(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, time.Hour, '0', 2);
	UTIL1_chcat(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, ':');
	UTIL1_strcatNum16sFormatted(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, time.Min, '0', 2);
	UTIL1_chcat(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, ':');
	UTIL1_strcatNum16sFormatted(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, time.Sec, '0', 2);

	UTIL1_chcat(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, ' ');
	UTIL1_strcat(errorLogLineBuf, ERRORLOG_LINEBUFFER_SIZE, message);

	FS_openFile(&errorLogFile,ERRORLOG_FILENAME);
	FS_writeLine(&errorLogFile,errorLogLineBuf);
	FS_closeFile(&errorLogFile);
}

static uint8_t ErrorLogFile_PrintList(CLS1_ConstStdIOType *io)
{
	uint8_t nofReadChars;
	FS_openFile(&errorLogFile,ERRORLOG_FILENAME);

	while(FS_readLine(&errorLogFile,errorLogLineBuf,ERRORLOG_LINEBUFFER_SIZE,&nofReadChars) == 0 &&
		  nofReadChars != 0)
	{
		CLS1_SendStr(errorLogLineBuf,io->stdErr);
	}

	FS_closeFile(&errorLogFile);
	return ERR_OK;
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


