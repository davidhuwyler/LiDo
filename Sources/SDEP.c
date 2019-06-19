/*
 * SDEP.c
 *
 *  Created on: Mar 4, 2019
 *      Author: dave
 */
#include "Platform.h"
#include "SDEP.h"
#include "CRC8.h"
#include "SDEPioHandler.h"
#include "AppDataFile.h"
#include "RTC.h"
#include "FileSystem.h"
#include "Shell.h"
#include "SDEPpendingAlertsBuffer.h"
#include "ErrorLogFile.h"
#include "PowerManagement.h"
#include "UI.h"

static LDD_TDeviceData* CRCdeviceDataHandle;
static LDD_TUserData *  CRCuserDataHandle;

static bool ongoingSDEPmessageProcessing = FALSE;

uint8_t SDEP_SendMessage(SDEPmessage_t* response)
{
	SDEPio_SendChar(response->type);
	SDEPio_SendChar((uint8_t)response->cmdId);
	SDEPio_SendChar((uint8_t)(response->cmdId>>8));
	SDEPio_SendChar(response->payloadSize);
	SDEPio_SendData(response->payload,response->payloadSize);
	SDEPio_SendChar(crc8_SDEPcrc(response));
	return ERR_OK;
}

uint8_t SDEP_ExecureCommand(SDEPmessage_t* command)
{
	uint8_t err = ERR_OK;
	const unsigned char *p;
	static uint8_t outputBuf[SDEP_MESSAGE_MAX_PAYLOAD_BYTES];
	static SDEPmessage_t answer;
	answer.type = SDEP_TYPEBYTE_RESPONSE;
	answer.cmdId = command->cmdId;
	answer.payload = outputBuf;

	int32_t sint32Param;
	uint8_t uint8param;
	uint16_t uint16Param;
	liDoSample_t sample;
	CLS1_ConstStdIOTypePtr io;

	*(command->payload + command->payloadSize)='\0'; //Add string termination (Overwrites CRC Byte)

	switch(command->cmdId)
	{
	case SDEP_CMDID_GET_ID:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[1][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		answer.payloadSize = UTIL1_strlen(answer.payload);
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_NAME:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[0][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		answer.payloadSize = UTIL1_strlen(answer.payload);
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_VERSION:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[2][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		answer.payloadSize = UTIL1_strlen(answer.payload);
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_SAMPLE:
		RTC_getTimeUnixFormat(&sint32Param);
		APP_getCurrentSample(&sample,sint32Param,TRUE);
		answer.payload[0] = (uint8_t)sample.unixTimeStamp;
		answer.payload[1] = (uint8_t)(sample.unixTimeStamp>>8);
		answer.payload[2] = (uint8_t)(sample.unixTimeStamp>>16);
		answer.payload[3] = (uint8_t)(sample.unixTimeStamp>>24);
		answer.payload[4] = (uint8_t)sample.lightGain;
		answer.payload[5] = (uint8_t)sample.lightIntTime;
		answer.payload[6] = (uint8_t)sample.lightChannelX;
		answer.payload[7] = (uint8_t)(sample.lightChannelX>>8);
		answer.payload[8] = (uint8_t)sample.lightChannelY;
		answer.payload[9] = (uint8_t)(sample.lightChannelY>>8);
		answer.payload[10] = (uint8_t)sample.lightChannelZ;
		answer.payload[11] = (uint8_t)(sample.lightChannelZ>>8);
		answer.payload[12] = (uint8_t)sample.lightChannelIR;
		answer.payload[13] = (uint8_t)(sample.lightChannelIR>>8);
		answer.payload[14] = (uint8_t)sample.lightChannelB440;
		answer.payload[15] = (uint8_t)(sample.lightChannelB440>>8);
		answer.payload[16] = (uint8_t)sample.lightChannelB490;
		answer.payload[17] = (uint8_t)(sample.lightChannelB490>>8);
		answer.payload[18] = (uint8_t)sample.accelX;
		answer.payload[19] = (uint8_t)sample.accelY;
		answer.payload[20] = (uint8_t)sample.accelZ;
		answer.payload[21] = (uint8_t)sample.temp;
		answer.payload[22] = (uint8_t)sample.crc;
		answer.payloadSize = LIDO_SAMPLE_SIZE;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_FILELIST:
	{
		SDEPio_getSDEPfileIO(&io);
		if(command->payloadSize == 0)
		{
			FS_FileList(NULL,io);
		}
		else
		{
			FS_FileList(command->payload,io);
		}
		SDEPio_HandleFileCMDs(SDEP_CMDID_GET_FILELIST);
		return ERR_OK;
	}

	case SDEP_CMDID_GET_FILE:
	{
		SDEPio_getSDEPfileIO(&io);
		return SDEPio_SetReadFileCMD(command->payload);
//		if(FS_ReadFile(command->payload,TRUE,SDEP_MESSAGE_MAX_PAYLOAD_BYTES +1,io) == ERR_FAILED)
//		{
//			break;
//		}
//		return ERR_OK;
	}

	case SDEP_CMDID_GET_SAMPLE_INT:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[3][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_RTC:
		RTC_getTimeUnixFormat(&sint32Param);
		answer.payload[0] = (uint8_t) sint32Param;
		answer.payload[1] = (uint8_t) (sint32Param>>8);
		answer.payload[2] = (uint8_t) (sint32Param>>16);
		answer.payload[3] = (uint8_t) (sint32Param>>24);
		answer.payloadSize = 4;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_EN_SAMPLE:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_LIGHTGAIN:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[5][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_LIGHTINTTIME:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[6][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_LIGHTWAITTIME:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[7][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_GET_EN_AUTOGAIN:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[8][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;

#if PL_CONFIG_HAS_BATT_ADC
	case SDEP_CMDID_GET_VOLTAGE:
		uint16Param =  PowerManagement_getBatteryVoltage();
		RTC_getTimeUnixFormat(&sint32Param);
		answer.payload[0] = (uint8_t) (uint16Param);
		answer.payload[1] = (uint8_t) (uint16Param>>8);
		answer.payloadSize = 2;
		SDEP_SendMessage(&answer);
		return ERR_OK;
#endif

	case SDEP_CMDID_GET_EN_SAMPLE_AUTO_OFF:
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[9][0], answer.payload ,SDEP_MESSAGE_MAX_PAYLOAD_BYTES);
		if(UTIL1_xatoi((const unsigned char **)&answer.payload,&sint32Param) != ERR_OK)
		{
			break;
		}
		answer.payload[0] = (uint8_t)  sint32Param;
		answer.payloadSize = 1;
		SDEP_SendMessage(&answer);
		return ERR_OK;


	case SDEP_CMDID_SET_ID:
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[1][0],command->payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_NAME:
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[0][0],command->payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_SAMPLE_INT:
		uint8param = command->payload[0];
		if(uint8param<1 ||uint8param>100)
		{
			break;
		}
		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[3][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_RTC:
		sint32Param  = command->payload[0];
		sint32Param |= command->payload[1]<<8 ;
		sint32Param |= command->payload[2]<<16 ;
		sint32Param |= command->payload[3]<<24 ;
		RTC_setTimeUnixFormat(sint32Param);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_EN_SAMPLE:
		uint8param = command->payload[0];
		if(uint8param<0 ||uint8param>1)
		{
			break;
		}

		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_LIGHTGAIN:
		uint8param = command->payload[0];
		if(uint8param<0 ||uint8param>3)
		{
			break;
		}

		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[5][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_LIGHTINTTIME:
		uint8param = command->payload[0];
		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[6][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_LIGHTWAITTIME:
		uint8param = command->payload[0];
		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[7][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_EN_AUTOGAIN:
		uint8param = command->payload[0];
		if(uint8param<0 ||uint8param>1)
		{
			break;
		}
		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[8][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SET_EN_SAMPLE_AUTO_OFF:
		uint8param = command->payload[0];
		if(uint8param<0 ||uint8param>1)
		{
			break;
		}
		UTIL1_Num8uToStr(answer.payload,SDEP_MESSAGE_MAX_PAYLOAD_BYTES,uint8param);
		AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[9][0],answer.payload);
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_SELFTEST:
		//Check Sensors & IIC
		sint32Param = 0;
		p = outputBuf;
		err = APP_getCurrentSample(&sample,sint32Param,TRUE);
		sample.temp &= ~0x80;  //Delete UserButton marker if there
		if(!(sample.temp > 0 && sample.temp < 80 )) {err = ERR_FAILED;}

		//Check FileSystem, MiniINI & SPIF
		AppDataFile_GetStringValue(APPDATA_KEYS_AND_DEV_VALUES[3][0], (uint8_t*)p ,25); //Read Sampleintervall from SPIF
		UTIL1_ScanDecimal8uNumber(&p, &uint8param);
		if(!(uint8param >= 1 && uint8param <= 100 )) {err = ERR_FAILED;}//Check Sampleintervall

#if PL_CONFIG_HAS_BATT_ADC
		//Check the Battery State
		uint16Param = PowerManagement_getBatteryVoltage();
		if(!(uint16Param >= 2978 && uint16Param <= 4200 )) {err = ERR_FAILED;}//Check Battery Voltagerange
#endif

		//If all OK, Turn on The RGB LED for the User to check...
		UI_LEDpulse(LED_W);

		if(err == ERR_OK)
		{
			answer.payloadSize = 0;
			answer.payload =0;
			SDEP_SendMessage(&answer);
		}
		else
		{
			answer.type = SDEP_TYPEBYTE_ERROR;
			answer.cmdId = SDEP_ERRID_SELFTEST_FAILED;
			answer.payloadSize = 0;
			answer.payload = 0;
			SDEP_SendMessage(&answer);
		}

		return err;

	case SDEP_CMDID_DELETE_FILE:
		if(FS_RemoveFile(command->payload,NULL) != ERR_OK)
		{
			break;
		}
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	case SDEP_CMDID_DEBUGCLI:
		SDEPio_switchIOtoSDEPio();
		SDEPio_SDEPtoShell(command->payload,command->payloadSize);
		return ERR_OK;

	case SDEP_CMDID_CLOSE_CONN:
		SHELL_requestDisabling();
		answer.payloadSize = 0;
		answer.payload =0;
		SDEP_SendMessage(&answer);
		return ERR_OK;

	default:
		answer.type = SDEP_TYPEBYTE_ERROR;
		answer.cmdId = SDEP_ERRID_INVALIDCMD;
		answer.payloadSize = 0;
		answer.payload = 0;
		SDEP_SendMessage(&answer);
		return ERR_FAILED;
	}

	answer.type = SDEP_TYPEBYTE_ERROR;
	answer.cmdId = SDEP_ERRID_INVALIDCMD;
	answer.payloadSize = 0;
	answer.payload = 0;
	SDEP_SendMessage(&answer);
	return ERR_FAILED;
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

	while(SDEPio_ReadChar(&inputBuf[inputBufPtr]) == ERR_OK && inputBufPtr<=SDEP_MESSAGE_MAX_NOF_BYTES)
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
	if(crc8_SDEPcrc(message) != message->crc)
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
		ongoingSDEPmessageProcessing = TRUE;
		SDEP_ExecureCommand(&message);
		ongoingSDEPmessageProcessing = FALSE;
	}
}

uint8_t SDEP_InitiateNewAlertWithMessage(uint16 CmdId,uint8_t* message)
{
	ErrorLogFile_LogError(CmdId,message);
	return SDEPpendingAlertsBuffer_Put(CmdId);
}

uint8_t SDEP_InitiateNewAlert(uint16 CmdId)
{
	ErrorLogFile_LogError(CmdId,"");
	return SDEPpendingAlertsBuffer_Put(CmdId);
}

uint8_t SDEP_SendPendingAlert(void)
{
	if(SDEPpendingAlertsBuffer_NofElements() != 0)
	{
		uint16 alertCmdId;
		SDEPpendingAlertsBuffer_Get(&alertCmdId);
		static SDEPmessage_t alert;
		alert.type = SDEP_TYPEBYTE_ALERT;
		alert.cmdId = alertCmdId;
		alert.payloadSize = 0;
		SDEP_SendMessage(&alert);
	}
	return ERR_OK;
}

uint8_t SDEP_Init(void)
{
	SDEPio_init();
}

