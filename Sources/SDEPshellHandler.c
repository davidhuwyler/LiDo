/*
 * SPEPshelHandler.c
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 */

#include "SDEPshellHandler.h"
#include "CLS1.h"
#include "CDC1.h"
#include "SDEPtoShellBuf.h"
#include "ShelltoSDEPBuf.h"

static CLS1_ConstStdIOTypePtr Std_io;
static CLS1_ConstStdIOType SDEP_ioNonPtr =
{
	(CLS1_StdIO_In_FctType) SDEP_ShellReadChar, /* stdin */
	(CLS1_StdIO_OutErr_FctType) SDEP_ShellSendChar, /* stdout */
	(CLS1_StdIO_OutErr_FctType) SDEP_ShellSendChar, /* stderr */
	SDEP_ShellKeyPressed /* if input is not empty */
};
static CLS1_ConstStdIOTypePtr SDEP_io = &SDEP_ioNonPtr;

static bool newSDEPshellMessage = false;


uint8_t SDEP_HandleShellCMDs(void)
{
	uint8_t ch;
	while(ShelltoSDEPBuf_NofElements())
	{
		ShelltoSDEPBuf_Get(&ch);
		SDEP_SendChar(ch);
	}

	return ERR_OK;
}


uint8_t SDEP_SendData(const uint8_t *data, uint8_t size)
{
	uint8_t err;
	for(int i = 0 ; i < size ; i++)
	{
		err = SDEP_SendChar(data[i]);
		if(err != ERR_OK)
		{
			return err;
		}
	}
	return ERR_OK;
}

uint8_t SDEP_SendChar(uint8_t c)
{
	CDC1_SendChar(c);
	return ERR_OK;
}

uint8_t SDEP_ReadChar(uint8_t *c)
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


uint8_t SDEP_SDEPtoShell(const uint8_t *str,uint8_t size)
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


uint8_t SDEP_ShellToSDEP(const uint8_t*str,uint8_t* size)
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

void SDEP_ShellReadChar(uint8_t *c)
{
	if(!SDEPtoShellBuf_NofElements())
	{
		*c = '\0';
	}
	SDEPtoShellBuf_Get(c); /* Received char */
}

void SDEP_ShellSendChar(uint8_t ch)
{
	ShelltoSDEPBuf_Put(ch);
}

bool SDEP_ShellKeyPressed(void)
{
	return (bool)SDEPtoShellBuf_NofElements();
}

void SDEPshellHandler_init(void)
{
	Std_io = CLS1_GetStdio();
}

void SDEPshellHandler_switchIOtoSDEPio(void)
{
	CLS1_SetStdio(SDEP_io);
	newSDEPshellMessage = true;
}

void SDEPshellHandler_switchIOtoStdIO(void)
{
	CLS1_SetStdio(Std_io);
}

bool SDEPshellHandler_NewSDEPmessageAvail(void)
{
	bool res = newSDEPshellMessage;
	newSDEPshellMessage = false;
	return res;
}

