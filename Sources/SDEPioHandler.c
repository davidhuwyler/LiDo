/*
 * SPEPshelHandler.c
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 */

#include "Platform.h"
#include "SDEPioHandler.h"
#include "SDEP.h"
#include "CDC1.h"
#include "FileSystem.h"
#include "SDEPtoShellBuf.h"
#include "ShelltoSDEPBuf.h"
#include "FileToSDEPBuf.h"

static CLS1_ConstStdIOTypePtr Std_io;
static CLS1_ConstStdIOType SDEP_ShellioNonPtr =
{
	(CLS1_StdIO_In_FctType) 	SDEPio_ShellReadChar, /* stdin */
	(CLS1_StdIO_OutErr_FctType) SDEPio_ShellSendChar, /* stdout */
	(CLS1_StdIO_OutErr_FctType) SDEPio_ShellSendChar, /* stderr */
	SDEPio_ShellKeyPressed /* if input is not empty */
};
static CLS1_ConstStdIOTypePtr SDEP_Shellio = &SDEP_ShellioNonPtr;

static CLS1_ConstStdIOType SDEP_FileioNonPtr =
{
	(CLS1_StdIO_In_FctType) 	SDEPio_FileReadChar, /* stdin */
	(CLS1_StdIO_OutErr_FctType) SDEPio_FileSendChar, /* stdout */
	(CLS1_StdIO_OutErr_FctType) SDEPio_FileSendChar, /* stderr */
	SDEPio_FileKeyPressed /* if input is not empty */
};
static CLS1_ConstStdIOTypePtr SDEP_Fileio = &SDEP_FileioNonPtr;

static bool newSDEPshellMessage = FALSE;

static lfs_file_t openFile;

static bool fileToRead = FALSE;

uint8_t SDEPio_HandleShellCMDs(void)
{
	static SDEPmessage_t message;
	static uint8_t outputBuf[SDEP_MESSAGE_MAX_PAYLOAD_BYTES];

	if(ShelltoSDEPBuf_NofElements())
	{
		message.payload = outputBuf;
		uint8_t ch;
		if(ShelltoSDEPBuf_NofElements()>SDEP_MESSAGE_MAX_PAYLOAD_BYTES)
		{
			message.payloadSize = SDEP_MESSAGE_MAX_PAYLOAD_BYTES | SDEP_PAYLOADBYTE_MORE_DATA_BIT;
			for(int i = 0 ; i < SDEP_MESSAGE_MAX_PAYLOAD_BYTES ; i++)
			{
				ShelltoSDEPBuf_Get(message.payload+i);
			}
		}
		else
		{
			message.payloadSize = ShelltoSDEPBuf_NofElements();
			for(int i = 0 ; i < message.payloadSize ; i++)
			{
				ShelltoSDEPBuf_Get(message.payload+i);
			}

		}
		message.cmdId = SDEP_CMDID_DEBUGCLI;
		message.type = SDEP_TYPEBYTE_RESPONSE;
		return SDEP_SendMessage(&message);
	}
	return ERR_RXEMPTY;
}

uint8_t SDEPio_SetReadFileCMD(uint8_t* filename)
{
	fileToRead = TRUE;
	return FS_openFile(&openFile,filename);
}

uint8_t SDEPio_HandleFileCMDs(uint16_t cmdId)
{
	static uint16_t ongoingCmdId = 0;
	static SDEPmessage_t message;
	static uint8_t outputBuf[SDEP_MESSAGE_MAX_PAYLOAD_BYTES];
	//static uint8_t outputBuf[1024];
	static bool nextFileAccessFromBeginning = TRUE;

	if(cmdId != 0)
	{
		message.cmdId = cmdId;
	}
	else if (ongoingCmdId != 0)
	{
		message.cmdId = ongoingCmdId;
	}

	if (fileToRead)
	{
		CLS1_ConstStdIOTypePtr io;
		SDEPio_getSDEPfileIO(&io);
		if(FS_ReadFile(&openFile,nextFileAccessFromBeginning,1024,io) != ERR_OK)
		{
			fileToRead = FALSE;
			FS_closeFile(&openFile);
			nextFileAccessFromBeginning = TRUE;
		}
		else
		{
			nextFileAccessFromBeginning = FALSE;
		}
		cmdId = SDEP_CMDID_GET_FILE;
		message.cmdId = SDEP_CMDID_GET_FILE;
	}

	if(FileToSDEPBuf_NofElements() >= 1024)
	{
		while(FileToSDEPBuf_NofElements()>SDEP_MESSAGE_MAX_PAYLOAD_BYTES)
		{
			message.payload = outputBuf;
			uint8_t ch;
			message.payloadSize = SDEP_MESSAGE_MAX_PAYLOAD_BYTES | SDEP_PAYLOADBYTE_MORE_DATA_BIT;
			for(int i = 0 ; i < SDEP_MESSAGE_MAX_PAYLOAD_BYTES ; i++)
			{
				FileToSDEPBuf_Get(message.payload+i);
			}
			ongoingCmdId = cmdId;
			message.type = SDEP_TYPEBYTE_RESPONSE;
			SDEP_SendMessage(&message);
		}
	}

	if(FileToSDEPBuf_NofElements())
	{

		message.payload = outputBuf;
		uint8_t ch;
		if(FileToSDEPBuf_NofElements()>SDEP_MESSAGE_MAX_PAYLOAD_BYTES)
		{
			message.payloadSize = SDEP_MESSAGE_MAX_PAYLOAD_BYTES | SDEP_PAYLOADBYTE_MORE_DATA_BIT;
			for(int i = 0 ; i < SDEP_MESSAGE_MAX_PAYLOAD_BYTES ; i++)
			{
				FileToSDEPBuf_Get(message.payload+i);
			}
			ongoingCmdId = cmdId;
		}
		else
		{
			message.payloadSize = FileToSDEPBuf_NofElements();
			for(int i = 0 ; i < message.payloadSize ; i++)
			{
				FileToSDEPBuf_Get(message.payload+i);
			}
			ongoingCmdId = 0;
		}
		message.type = SDEP_TYPEBYTE_RESPONSE;
		return SDEP_SendMessage(&message);
	}
	return ERR_RXEMPTY;
}

uint8_t SDEPio_SendData(const uint8_t *data, uint8_t size)
{
	uint8_t err;
	if((size & SDEP_PAYLOADBYTE_MORE_DATA_BIT) == SDEP_PAYLOADBYTE_MORE_DATA_BIT)
	{
		size = SDEP_MESSAGE_MAX_PAYLOAD_BYTES;
	}

	for(int i = 0 ; i < size ; i++)
	{
		err = SDEPio_SendChar(data[i]);
		if(err != ERR_OK)
		{
			return err;
		}
	}
	return ERR_OK;
}

uint8_t SDEPio_SendChar(uint8_t c)
{
	CDC1_SendChar(c);
	return ERR_OK;
}

uint8_t SDEPio_ReadChar(uint8_t *c)
{
	if(CDC1_GetCharsInRxBuf())
	{
		CDC1_GetChar(c);
		return ERR_OK;
	}
	else
	{
		return ERR_RXEMPTY;
	}
}


uint8_t SDEPio_SDEPtoShell(const uint8_t *str,uint8_t size)
{
	uint8_t err;
	for(int i = 0 ; i < size ; i++)
	{
		err = SDEPtoShellBuf_Put(*str);
		str++;

		if(err != ERR_OK)
		{
			return err;
		}
	}
	err = SDEPtoShellBuf_Put('\0');	//Shell String Termination
	return err;
}


uint8_t SDEPio_ShellToSDEP(const uint8_t*str,uint8_t* size)
{
	uint8_t err;
	*size = 0;
	while (*str != '\0' && *size<0x80)
	{
		err = ShelltoSDEPBuf_Put(*str++);
		size++;
		if(err != ERR_OK)
		{
			return err;
		}
	}
	return err;
}

/*
 * SDEP Shell Io function:
 */
void SDEPio_ShellReadChar(uint8_t *c)
{
	if(!SDEPtoShellBuf_NofElements())
	{
		*c = '\0';
	}
	SDEPtoShellBuf_Get(c); /* Received char */
}
void SDEPio_ShellSendChar(uint8_t ch)
{
	ShelltoSDEPBuf_Put(ch);
}
bool SDEPio_ShellKeyPressed(void)
{
	return (bool)SDEPtoShellBuf_NofElements();
}

/*
 * SDEP File Io function:
 */
void SDEPio_FileReadChar(uint8_t *c)
{
	*c = '\0';
}
void SDEPio_FileSendChar(uint8_t ch)
{
	FileToSDEPBuf_Put(ch);
}
bool SDEPio_FileKeyPressed(void)
{
	return (bool)FileToSDEPBuf_NofElements();
}


void SDEPio_getSDEPfileIO(CLS1_ConstStdIOTypePtr* io)
{
	*io = SDEP_Fileio;
}

void SDEPio_init(void)
{
	Std_io = CLS1_GetStdio();
}

void SDEPio_switchIOtoSDEPio(void)
{
	CLS1_SetStdio(SDEP_Shellio);
	CS1_CriticalVariable();
	CS1_EnterCritical();
	newSDEPshellMessage = TRUE;
	CS1_ExitCritical();
}

void SDEPio_switchIOtoStdIO(void)
{
	CLS1_SetStdio(Std_io);
}

bool SDEPio_NewSDEPmessageAvail(void)
{
	CS1_CriticalVariable();
	CS1_EnterCritical();
	bool res = newSDEPshellMessage;
	newSDEPshellMessage = FALSE;
	CS1_ExitCritical();
	return res;
}
