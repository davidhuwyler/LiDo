/*
 * SDEP.h
 *
 *  Created on: Mar 4, 2019
 *      Author: dave
 */

#ifndef SOURCES_SDEP_H_
#define SOURCES_SDEP_H_

#include <stdint.h>
#include <stddef.h>
#include "CLS1.h"

typedef struct
{
	uint8_t  type;
	uint16_t cmdId;
	uint8_t  payloadSize;
	uint8_t* payload;
	uint8_t  crc;
} SDEPmessage_t;

#define SDEP_MESSAGE_MAX_NOF_BYTES 131
#define SDEP_MESSAGE_MAX_PAYLOAD_BYTES 127
#define SDEP_PAYLOADBYTE_MORE_DATA_BIT 0x80

#define SDEP_TYPEBYTE_COMMAND  		0x10
#define SDEP_TYPEBYTE_RESPONSE  	0x20
#define SDEP_TYPEBYTE_ALERT  		0x40
#define SDEP_TYPEBYTE_ERROR  		0x80

#define SDEP_CMDID_GET_ID			0x0000 //Done
#define SDEP_CMDID_GET_NAME			0x0001 //Done
#define SDEP_CMDID_GET_VERSION		0x0002 //Done
#define SDEP_CMDID_GET_SAMPLE		0x0003 //Done
#define SDEP_CMDID_GET_FILELIST		0x0004 //Done
#define SDEP_CMDID_GET_FILE			0x0005 //Done
#define SDEP_CMDID_GET_EN_SAMPLE 	0x0006 //Done
#define SDEP_CMDID_GET_SAMPLE_INT	0x0007 //Done
#define SDEP_CMDID_GET_RTC			0x0008 //Done

#define SDEP_CMDID_SET_ID			0x0010 //Done
#define SDEP_CMDID_SET_NAME			0x0011 //Done
#define SDEP_CMDID_SET_SAMPLE_INT	0x0012 //Done
#define SDEP_CMDID_SET_RTC			0x0013 //Done
#define SDEP_CMDID_SET_EN_SAMPLE	0x0014 //Done

#define SDEP_CMDID_CALIBRATE		0x0100
#define SDEP_CMDID_SELFTEST			0x0101
#define SDEP_CMDID_DELETE_FILE		0x0102 //Done

#define SDEP_CMDID_DEBUGCLI			0x1000 //Done

#define SDEP_CMDID_CLOSE_CONN		0x2000

#define SDEP_ERRID_INVALIDCMD		0x0001 //Done
#define SDEP_ERRID_INVALIDPAYLOAD	0x0003 //Done
#define SDEP_ERRID_INVALIDCRC		0x0005 //Done

uint8_t SDEP_Parse(void);
uint8_t SDEP_Init(void);
uint8_t SDEP_SendMessage(SDEPmessage_t* response);

#endif /* SOURCES_SDEP_H_ */
