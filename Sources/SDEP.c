/*
 * SDEP.c
 *
 *  Created on: Mar 4, 2019
 *      Author: dave
 */


#include "SDEP.h"
#include "CRC8.h"
#include "SDEPshellHandler.h"


#define SDEP_TYPEBYTE_COMMAND  	0x10
#define SDEP_TYPEBYTE_RESPONSE  0x20
#define SDEP_TYPEBYTE_ALERT  	0x40
#define SDEP_TYPEBYTE_ERROR  	0x80
#define SDEP_CMDID_GETNAME		0x0001
#define SDEP_CMDID_SETNAME		0x0011
#define SDEP_CMDID_DEBUGCLI		0x1000

#define SDEP_ERRID_INVALIDCMD		0x0001
#define SDEP_ERRID_INVALIDPAYLOAD	0x0003
#define SDEP_ERRID_INVALIDCRC		0x0005


typedef struct
{
	uint8_t  type;
	uint16_t cmdId;
	uint8_t  payloadSize;
	uint8_t* payload;
	uint8_t  crc;
} SDEPmessage_t;

LDD_TDeviceData* CRCdeviceDataHandle;
LDD_TUserData *  CRCuserDataHandle;

#define SDEP_LIDO_NAMEBUFFER_SIZE 30
#define SDEP_LIDO_IDBUFFER_SIZE 30

static char liDoNameBuffer[SDEP_LIDO_NAMEBUFFER_SIZE];
static char liDoIDBuffer[SDEP_LIDO_IDBUFFER_SIZE];

static bool ongoingSDEPmessageProcessing = false;

//uint8_t SDEP_GetCRC(uint8_t typeByte, uint16 command, uint8_t payloadSize,const unsigned char *payload)
uint8_t SDEP_GetCRC(SDEPmessage_t* message)
{
	uint8_t crc,seed = 0;
	crc = crc8_bytecalc(message->type,&seed);
	crc = crc8_bytecalc((uint8_t)message->cmdId,&seed);
	crc = crc8_bytecalc((uint8_t)(message->cmdId>>8),&seed);
	crc = crc8_bytecalc(message->payloadSize,&seed);
	crc = crc8_messagecalc((unsigned char *)message->payload,message->payloadSize,&seed);
	return crc;
}

uint8_t SDEP_SendMessage(SDEPmessage_t* response)
{
	SDEP_SendChar(response->type);
	SDEP_SendChar((uint8_t)response->cmdId);
	SDEP_SendChar((uint8_t)(response->cmdId>>8));
	SDEP_SendChar(response->payloadSize);
	SDEP_SendData(response->payload,response->payloadSize);
	SDEP_SendChar(SDEP_GetCRC(response));
}

uint8_t SDEP_ExecureCommand(SDEPmessage_t* message)
{
	static uint8_t outputBuf[SDEP_MESSAGE_MAX_NOF_BYTES];
	static SDEPmessage_t answer;
	answer.type = SDEP_TYPEBYTE_RESPONSE;
	answer.cmdId = message->cmdId;

	switch(message->cmdId)
	{
	case SDEP_CMDID_GETNAME:
		answer.payloadSize = 20;
		answer.payload =liDoNameBuffer;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SETNAME:
		UTIL1_strcpy(liDoNameBuffer,SDEP_LIDO_NAMEBUFFER_SIZE,message->payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_DEBUGCLI:
		SDEPshellHandler_switchIOtoSDEPio();
		SDEP_SDEPtoShell(message->payload,message->payloadSize);
		return ERR_OK;

	default:
		answer.type = SDEP_TYPEBYTE_ERROR;
		answer.cmdId = SDEP_ERRID_INVALIDCMD;
		answer.payloadSize = 0;
		answer.payload = 0;
		SDEP_SendMessage(&answer);
		return ERR_FAILED;
	}
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"SDEP", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t SDEP_ReadSDEPMessage(SDEPmessage_t* message)
{
	static uint8_t inputBuf[SDEP_MESSAGE_MAX_NOF_BYTES+1];
	static uint8_t inputBufPtr = 0;
	static SDEPmessage_t error;
	error.type = SDEP_TYPEBYTE_ERROR;
	error.payloadSize = 0;
	error.payload = 0;

	if(ongoingSDEPmessageProcessing)
	{
		return ERR_BUSY;
	}

	while(SDEP_ReadChar(&inputBuf[inputBufPtr]) == ERR_OK && inputBufPtr<=SDEP_MESSAGE_MAX_NOF_BYTES)
	{
		inputBufPtr++;
	}

	if(inputBufPtr == 0)
	{
		return ERR_RXEMPTY;
	}

	//Check TypeByte
	if(!(inputBuf[0] == SDEP_TYPEBYTE_COMMAND | inputBuf[0] == SDEP_TYPEBYTE_RESPONSE | inputBuf[0] == SDEP_TYPEBYTE_ERROR | inputBuf[0] == SDEP_TYPEBYTE_ALERT))
	{
		inputBufPtr = 0;
		error.cmdId = SDEP_ERRID_INVALIDCMD;
		SDEP_SendMessage(&error);
		return ERR_PARAM_COMMAND;
	}

	message->type = inputBuf[0];
	message->payloadSize = inputBuf[3];
	message->cmdId = (uint16)(inputBuf[1] | inputBuf[2]<<8);
	message->crc = inputBuf[4+message->payloadSize];
	message->payload = inputBuf+4;

	//Check CRC


//	uint8_t crc = 0;
//	for(int i = 0; i < 4+message->payloadSize; i++)
//	{
//		crc = CRC8(crc,inputBuf[i]);
//	}

	//uint8_t crc = getCRC(inputBuf,3+message->payloadSize);
	//if(crc != message->crc)
	//if(SDEP_GetCRC(message->type, message->cmdId , message->payloadSize, message->payload) != message->crc)
	if(SDEP_GetCRC(message) != message->crc)
	{
		message->type = 0;
		message->payloadSize = 0;
		message->cmdId = 0;
		message->crc = 0;
		message->payload = 0;

		error.cmdId = SDEP_ERRID_INVALIDCRC;
		SDEP_SendMessage(&error);

		if(inputBufPtr>=message->payloadSize)
		{
			inputBufPtr = 0;
		}
		return ERR_PARAM_COMMAND;
	}

	inputBufPtr = 0;
	return ERR_OK;
}

uint8_t SDEP_Parse(void)
{
	SDEPmessage_t message;
	if(SDEP_ReadSDEPMessage(&message) == ERR_OK)
	{
		ongoingSDEPmessageProcessing = true;
		SDEP_ExecureCommand(&message);
		ongoingSDEPmessageProcessing = false;
	}
}

uint8_t SDEP_Init(void)
{
	SDEPshellHandler_init();
}

