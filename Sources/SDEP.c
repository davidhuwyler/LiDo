/*
 * SDEP.c
 *
 *  Created on: Mar 4, 2019
 *      Author: dave
 */

#include "SDEP.h"
#include "CRC1.h"

#define SDEP_TYPEBYTE_COMMAND  	0x10
#define SDEP_TYPEBYTE_RESPONSE  0x20
#define SDEP_TYPEBYTE_ERROR  	0x40

#define SDEP_CMDID_GETNAME		0x0001

#define SDEP_CMDID_SETNAME		0x0011


LDD_TDeviceData* CRCdeviceDataHandle;
LDD_TUserData *  CRCuserDataHandle;


#define SDEP_LIDO_NAMEBUFFER_SIZE 30
#define SDEP_LIDO_IDBUFFER_SIZE 30

static char liDoNameBuffer[SDEP_LIDO_NAMEBUFFER_SIZE];
static char liDoIDBuffer[SDEP_LIDO_IDBUFFER_SIZE];


uint8_t SDEP_ExecureCommand(uint16_t command, uint8_t payloadSize, uint8_t receivedCRC, const unsigned char *payload,CLS1_ConstStdIOType *io)
{
	switch(command)
	{
	case SDEP_CMDID_GETNAME:
		if(io != NULL)
		{
			CLS1_SendStr(liDoNameBuffer, io->stdErr);
		}
		return ERR_OK;

	case SDEP_CMDID_SETNAME:
		UTIL1_strcpy(liDoNameBuffer,SDEP_LIDO_NAMEBUFFER_SIZE,payload);
		return ERR_OK;

	default:
		if(io != NULL)
		{
			CLS1_SendStr(" Unknown Command\r\n", io->stdErr);
		}
		return ERR_FAILED;


	}

}

uint8_t SDEP_GetCRC(uint8_t typeByte, uint16 command, uint8_t payloadSize,const unsigned char *payload)
{
	uint8_t crc;
	CRC1_ResetCRC(CRCdeviceDataHandle);
	crc = CRC1_GetCRC8(CRCdeviceDataHandle,typeByte);
	crc = CRC1_GetCRC8(CRCdeviceDataHandle,(uint8_t)command);
	crc = CRC1_GetCRC8(CRCdeviceDataHandle,(uint8_t)(command>>8));
	crc = CRC1_GetCRC8(CRCdeviceDataHandle,payloadSize);
	for(int i = 0 ; i < payloadSize ; i ++)
	{
		crc = CRC1_GetCRC8(CRCdeviceDataHandle,payload[i]);
	}
	return crc;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"SDEP", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}


uint8_t SDEP_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io)
{
  uint8_t res = ERR_OK;
  const uint8_t *p;

	if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP) == 0|| UTIL1_strcmp((char*)cmd, "SDEP help") == 0)
	{
		CLS1_SendHelpStr((unsigned char*) "SDEP",	(const unsigned char*) "Group of SDEP commands used to transfer Data/config\r\n",	io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  help|status",	(const unsigned char*) "Print help or status information\r\n",	io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  0011 <name>",	(const unsigned char*) "SetName\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  0001",	(const unsigned char*) "GetName\r\n", io->stdOut);
		*handled = TRUE;
		return ERR_OK;
	}
	else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS) == 0)	|| (UTIL1_strcmp((char*)cmd, "SDEP status") == 0)) {
		*handled = TRUE;
		return PrintStatus(io);
	}
	else if (UTIL1_strncmp((char* )cmd, "SDEP 0001",	sizeof("SDEP 0001") - 1) == 0)	/* GetName */
	{
		*handled = TRUE;
		p = cmd + sizeof("SDEP 0001 ") - 1;
		uint16_t command = 0x0001;
		uint8_t payloadLength = UTIL1_strlen(p);
		uint8_t crc = SDEP_GetCRC(SDEP_TYPEBYTE_COMMAND,command,payloadLength,p);
		return res = SDEP_ExecureCommand(command, payloadLength,crc,p,io);
	}
	else if (UTIL1_strncmp((char* )cmd, "SDEP 0011 ",	sizeof("SDEP 0001 ") - 1) == 0)	/* SetName */
	{
		*handled = TRUE;
		p = cmd + sizeof("SDEP 0011 ") - 1;
		uint16_t command = 0x0011;
		uint8_t payloadLength = UTIL1_strlen(p);
		uint8_t crc = SDEP_GetCRC(SDEP_TYPEBYTE_COMMAND,command,payloadLength,p);
		return res = SDEP_ExecureCommand(command, payloadLength,crc,p,io);
	}
	return res;
}

uint8_t SDEP_ParseSilentCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io)
{
  uint8_t res = ERR_OK;
  if (cmd[0] == SDEP_TYPEBYTE_COMMAND)
  {
	  *handled = TRUE;
//	  char size = Rx1_NofElements();
//	  char header[4];
//	  Rx1_Getn(header,4);
	  return res = SDEP_ExecureCommand((uint16_t)cmd[1]|(cmd[2]<<8), (uint8_t) cmd[3], (uint8_t) *(cmd+4+cmd[3]),cmd+4,NULL);
  }
  return res;
}

uint8_t SDEP_Init(void)
{
	CRCdeviceDataHandle = CRC1_Init(CRCuserDataHandle);
}

