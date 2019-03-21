/*
 * SPIF.c
 *
 *  Created on: Mar 1, 2019
 *      Author: dave
 *
 *  Driver for the 64MB MX25L51245G SPIF from Macronix
 *  This Driver uses the 4Byte address mode of the Chip
 *
 *  Based on https://github.com/ErichStyger/mcuoneclipse/blob/master/Examples/KDS/tinyK20/tinyK20_LittleFS_W25Q128/Sources/W25Q128.c
 *
 *  2048  Blocks a 32kByte  (Blocks are erasable)
 * 	16384 Sectors a 4kByte	(Sectors are erasable)
 * 		  Pages a 256Byte 	(Pages are not erarable one by one)
 */

#include "SPIF.h"
#include "Shell.h"


#include "SM1.h"
#include "UTIL1.h"
#include "CLS1.h"
#include "WAIT1.h"

#include "PIN_SPIF_PWR.h"
#include "PIN_SPIF_CS.h"
#include "PIN_SPIF_RESET.h"
#include "PIN_SPIF_WP.h"

#define SPIF_SPI_CMD_READ_DATA 0x13			//4Byte address normal read
#define SPIF_SPI_CMD_PROGRAM_PAGE 0x12		//4Byte address

#define SPIF_SPI_CMD_ERASE_32K_BLOCK 0x5C
#define SPIF_SPI_CMD_ERASE_64K_BLOCK 0xDC
#define SPIF_SPI_CMD_ERASE_4K_SECTOR 0x21
#define SPIF_SPI_CMD_ERASE_CHIP 0xC7 //or 0x60 whats the difference??

#define SPIF_SPI_CMD_WRITE_ENABLE 0x06
#define SPIF_SPI_CMD_WRITE_DISABLE 0x04

#define SPIF_SPI_CMD_READ_STATUS 0x05 //Status Register in Datasheet p36
#define SPIF_SPI_CMD_READ_CONFIG 0x15
#define SPIF_SPI_CMD_WRITE_STAT_CONF 0x01

#define SPIF_SPI_CMD_ENABLE_4BYTE_ADD 0xB7

#define SPIF_SPI_CMD_ENABLE_DEEP_SLEEP 0xB9
#define SPIF_SPI_CMD_DISABLE_DEEP_SLEEP 0xAB

#define SPIF_SPI_CMD_ENABLE_DEEP_SLEEP 0xB9
#define SPIF_SPI_CMD_DISABLE_DEEP_SLEEP 0xAB

#define SPIF_SPI_CMD_READ_ID 0x9F
#define SPIF_SPI_DEV_ID0_MANUFACTURER 0xC2
#define SPIF_SPI_DEV_ID1_MEM_TYPE 0x20
#define SPIF_SPI_DEV_ID2_MEM_DENS 0x1A

static LDD_TDeviceData* SPIdeviceHandle;
static LDD_TUserData*  SPIuserDataHandle;

/* W25Q128 chip select is LOW active */
#define SPIF_CS_ENABLE()   PIN_SPIF_CS_ClrVal()
#define SPIF_CS_DISABLE()  PIN_SPIF_CS_SetVal()

static uint8_t rxDummy; /* dummy byte if we do not need the result. Needed to read from SPI register. */
#define SPI_WRITE(write)            \
   { \
     while(SM1_SendChar(write)!=ERR_OK) {} \
     while(SM1_RecvChar(&rxDummy)!=ERR_OK) {} \
   }
#define SPI_WRITE_READ(write, readP) \
   { \
     while(SM1_SendChar(write)!=ERR_OK) {} \
     while(SM1_RecvChar(readP)!=ERR_OK) {} \
   }

uint8_t SPIF_ReadStatus(uint8_t *status)
{
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_READ_STATUS);
	  SPI_WRITE_READ(0, status);
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}

bool SPIF_isBusy(void)
{
	  uint8_t status;
	  SPIF_ReadStatus(&status);
	  return (status&1);  //Bit1 of StatusReg = WIP (Write in progress)
}

void SPIF_WaitIfBusy(void)
{
	  while(SPIF_isBusy())
	  {
	    WAIT1_Waitms(1);
	  }
}

uint8_t SPIF_Read(uint32_t address, uint8_t *buf, size_t bufSize)
{
	  size_t i;
	  SPIF_WaitIfBusy();
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_READ_DATA);
	  SPI_WRITE(address>>24);
	  SPI_WRITE(address>>16);
	  SPI_WRITE(address>>8);
	  SPI_WRITE(address);
	  for(i=0;i<bufSize;i++) {
	    SPI_WRITE_READ(0, &buf[i]);
	  }
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}

uint8_t SPIF_EraseAll(void)
{
	  SPIF_WaitIfBusy();
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_WRITE_ENABLE);
	  SPIF_CS_DISABLE();
	  WAIT1_Waitus(1);

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_ERASE_CHIP);
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}


uint8_t SPIF_EraseSector4K(uint32_t address)
{
	  SPIF_WaitIfBusy();
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_WRITE_ENABLE);
	  SPIF_CS_DISABLE();
	  WAIT1_Waitus(1);

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_ERASE_4K_SECTOR);
	  SPI_WRITE(address>>24);
	  SPI_WRITE(address>>16);
	  SPI_WRITE(address>>8);
	  SPI_WRITE(address);
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}

uint8_t SPIF_EraseBlock32K(uint32_t address)
{
	  SPIF_WaitIfBusy();
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_WRITE_ENABLE);
	  SPIF_CS_DISABLE();
	  WAIT1_Waitus(1);

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_ERASE_32K_BLOCK);
	  SPI_WRITE(address>>24);
	  SPI_WRITE(address>>16);
	  SPI_WRITE(address>>8);
	  SPI_WRITE(address);
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}

uint8_t SPIF_EraseBlock64K(uint32_t address)
{
	  SPIF_WaitIfBusy();
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_WRITE_ENABLE);
	  SPIF_CS_DISABLE();
	  WAIT1_Waitus(1);

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_ERASE_64K_BLOCK);
	  SPI_WRITE(address>>24);
	  SPI_WRITE(address>>16);
	  SPI_WRITE(address>>8);
	  SPI_WRITE(address);
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}

/*!
 * Program a page with data
 * \param address, should be aligned on page (256 bytes) if programming 256 bytes
 * \param data pointer to data
 * \param dataSize size of data in bytes, max 256
 * \return error code, ERR_OK for no error
 */
uint8_t SPIF_ProgramPage(uint32_t address, const uint8_t *data, size_t dataSize)
{
	  SPIF_WaitIfBusy();
	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_WRITE_ENABLE);
	  SPIF_CS_DISABLE();
	  WAIT1_Waitus(1);

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_PROGRAM_PAGE);
	  SPI_WRITE(address>>24);
	  SPI_WRITE(address>>16);
	  SPI_WRITE(address>>8);
	  SPI_WRITE(address);
	  while(dataSize>0) {
	    SPI_WRITE(*data);
	    dataSize--;
	    data++;
	  }
	  SPIF_CS_DISABLE();
	  return ERR_OK;
}


uint8_t SPIF_GetCapacity(const uint8_t *id, uint32_t *capacity)
{
	  uint32_t n = 0x100000; // unknown chips, default to 1 MByte

	  if (id[2] >= 16 && id[2] <= 31) {
	    n = 1ul << id[2];
	  } else if (id[2] >= 32 && id[2] <= 37) {
	    n = 1ul << (id[2] - 6);
	  } else if ((id[0]==0 && id[1]==0 && id[2]==0) || (id[0]==255 && id[1]==255 && id[2]==255)) {
	    *capacity = 0;
	    return ERR_FAILED;
	  }
	  *capacity = n;
	  return ERR_OK;
}

//Serial number not supported by the MX25L51245G Chip
//uint8_t SPIF_ReadSerialNumber(uint8_t *buf, size_t bufSize) {
//  int i;
//
//  if (bufSize<W25_SERIAL_BUF_SIZE) {
//    return ERR_OVERFLOW; /* buffer not large enough */
//  }
//
//  W25_CS_ENABLE();
//  SPI_WRITE(W25_CMD_GET_SERIAL);
//  for(i=0;i<4;i++) {
//    SPI_WRITE(0); /* 4 dummy bytes */
//  }
//  for(i=0; i<W25_SERIAL_BUF_SIZE; i++) {
//    SPI_WRITE_READ(0, &buf[i]);
//  }
//  W25_CS_DISABLE();
//  return ERR_OK;
//}

uint8_t SPIF_ReadID(uint8_t *buf, size_t bufSize)
{
	  if (bufSize<SPIF_ID_BUF_SIZE)
	  {
		  return ERR_OVERFLOW; /* buffer not large enough */
	  }

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_READ_ID);

	  // MX25L51245G should report 0xC2 20 1A
	  SPI_WRITE_READ(0, &buf[0]);
	  SPI_WRITE_READ(0, &buf[1]);
	  SPI_WRITE_READ(0, &buf[2]);
	  SPIF_CS_DISABLE();

	  uint8_t id0 = buf[0];
	  uint8_t id1 = buf[1];
	  uint8_t id2 = buf[2];

	  if (id0==SPIF_SPI_DEV_ID0_MANUFACTURER && id1==SPIF_SPI_DEV_ID1_MEM_TYPE && id2==SPIF_SPI_DEV_ID2_MEM_DENS)
	  {
		  return ERR_OK;
	  }

//	  if (buf[0]==SPIF_SPI_DEV_ID0_MANUFACTURER && buf[1]==SPIF_SPI_DEV_ID1_MEM_TYPE && buf[2]==SPIF_SPI_DEV_ID2_MEM_DENS)
//	  {
//		  return ERR_OK;
//	  }
	  return ERR_FAILED; /* not expected part */
}

static uint8_t SPIF_PrintStatus(CLS1_ConstStdIOType *io)
{
	uint8_t buf[60];
	uint8_t id[SPIF_ID_BUF_SIZE] = { 0, 0, 0 };
	uint8_t res, status;
	uint32_t capacity;
	int i;

	CLS1_SendStatusStr((const unsigned char*) "SPIF",(const unsigned char*) "\r\n", io->stdOut);

	res = SPIF_ReadID(id, sizeof(id)); /* check ID */
	if (res == ERR_OK)
	{
		buf[0] = '\0';
		UTIL1_strcatNum8Hex(buf, sizeof(buf), id[0]);
		UTIL1_chcat(buf, sizeof(buf), ' ');
		UTIL1_strcatNum8Hex(buf, sizeof(buf), id[1]);
		UTIL1_chcat(buf, sizeof(buf), ' ');
		UTIL1_strcatNum8Hex(buf, sizeof(buf), id[2]);
		if (id[0] == SPIF_SPI_DEV_ID0_MANUFACTURER
				&& id[1] == SPIF_SPI_DEV_ID1_MEM_TYPE
				&& id[2] == SPIF_SPI_DEV_ID2_MEM_DENS)
		{
			UTIL1_strcat(buf, sizeof(buf), " (Macronix MX25L51245G)\r\n");
		}
		else
		{
			UTIL1_strcat(buf, sizeof(buf), " (UNKNOWN)\r\n");
		}
	}
	else
	{
		UTIL1_strcpy(buf, sizeof(buf), "ERROR\r\n");
	}
	CLS1_SendStatusStr((const unsigned char*) "  ID", buf, io->stdOut);

	res = SPIF_GetCapacity(id, &capacity);
	if (res == ERR_OK)
	{
		buf[0] = '\0';
		UTIL1_strcatNum32u(buf, sizeof(buf), capacity);
		UTIL1_strcat(buf, sizeof(buf), " bytes\r\n");
	}
	else
	{
		UTIL1_strcpy(buf, sizeof(buf), "ERROR\r\n");
	}
	CLS1_SendStatusStr((const unsigned char*) "  Capacity", buf, io->stdOut);

	res = SPIF_ReadStatus(&status);
	if (res == ERR_OK)
	{
		UTIL1_strcpy(buf, sizeof(buf), "0x");
		UTIL1_strcatNum8Hex(buf, sizeof(buf), status);
		UTIL1_strcat(buf, sizeof(buf), " SRWD:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 7) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " QE:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 6) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " BP3:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 5) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " BP2:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 4) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " BP1:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 3) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " BP0:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 2) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " WEL:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 1) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), " BUSY:");
		UTIL1_strcat(buf, sizeof(buf), status & (1 << 0) ? "1" : "0");
		UTIL1_strcat(buf, sizeof(buf), "\r\n");
	}
	else
	{
		UTIL1_strcpy(buf, sizeof(buf), "ERROR\r\n");
	}
	CLS1_SendStatusStr((const unsigned char*) "  Status", buf, io->stdOut);
	return ERR_OK;
}

static uint8_t ReadBytes(void *hndl, uint32_t address, uint8_t *buf, size_t bufSize)
{
	  (void)hndl; /* not used */
	  return SPIF_Read(address, buf, bufSize);
}

uint8_t SPIF_ParseCommand(const unsigned char* cmd, bool *handled, const CLS1_StdIOType *io)
{
	const unsigned char *p;
	uint32_t val, end;
	uint32_t res;
	uint8_t data[32];
	int i;
	uint8_t buf[8];

	if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP) == 0|| UTIL1_strcmp((char*)cmd, "SPIF help") == 0) {CLS1_SendHelpStr((unsigned char*) "SPIF",(const unsigned char*) "Group of SPI Flash commands\r\n",io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  help|status",(const unsigned char*) "Print help or status information\r\n",io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  read <start> <end>",(const unsigned char*) "Read memory from <start> to <end> address\r\n",io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  erase all",(const unsigned char*) "Erase all\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  erase 4k <addr>",(const unsigned char*) "Erase a 4K sector\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  erase 32k <addr>",(const unsigned char*) "Erase a 32K block\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  erase 64k <addr>",(const unsigned char*) "Erase a 64K block\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  write <addr> <data>",(const unsigned char*) "Write to page (max 32 bytes data, separated by spaces)\r\n",io->stdOut);
		*handled = TRUE;
		return ERR_OK;
	}
	else if (UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS) == 0	|| UTIL1_strcmp((char*)cmd, "SPIF status") == 0)
	{
		*handled = TRUE;
		return SPIF_PrintStatus(io);
	}
	else if (UTIL1_strncmp((char* )cmd, "SPIF read ",sizeof("SPIF read ") - 1)== 0)
	{
		*handled = TRUE;
		p = cmd + sizeof("SPIF read ") - 1;
		res = UTIL1_xatoi(&p, &val);
		if (res != ERR_OK)
		{
			CLS1_SendStr("wrong start address\r\n", io->stdErr);
			return ERR_FAILED;
		}
		else
		{
			res = UTIL1_xatoi(&p, &end);
			if (res != ERR_OK)
			{
				CLS1_SendStr("wrong end address\r\n", io->stdErr);
				return ERR_FAILED;
			}
			res = CLS1_PrintMemory(NULL, val, end, 3, 16, ReadBytes, io);
			if (res != ERR_OK)
			{
				CLS1_SendStr("memory read failed\r\n", io->stdErr);
				return ERR_FAILED;
			}
			return ERR_OK;
		}
	}
	else if (UTIL1_strncmp((char* )cmd, "SPIF write ",sizeof("SPIF write ") - 1)== 0)
	{
		*handled = TRUE;
		p = cmd + sizeof("SPIF write ") - 1;
		res = UTIL1_xatoi(&p, &val);
		if (res != ERR_OK)
		{
			CLS1_SendStr("wrong address\r\n", io->stdErr);
			return ERR_FAILED;
		}
		else
		{
			for (i = 0; i < sizeof(data); i++)
			{
				uint32_t v;
				res = UTIL1_xatoi(&p, &v);
				if (res != ERR_OK)
				{
					break;
				}
				data[i] = v;
			}
			if (i > 0)
			{
				res = SPIF_ProgramPage(val, data, i);
				if (res != ERR_OK)
				{
					CLS1_SendStr("failed programming page\r\n", io->stdErr);
					return ERR_FAILED;
				}
			}
		}
		return ERR_OK;
	}
	else if (UTIL1_strcmp((char*)cmd, "SPIF erase all") == 0)
	{
		*handled = TRUE;
		res = SPIF_EraseAll();
		if (res != ERR_OK)
		{
			CLS1_SendStr("failed erasing all memory\r\n", io->stdErr);
			return ERR_FAILED;
		}
		return ERR_OK;
	}
	else if (UTIL1_strncmp((char* )cmd, "SPIF erase 4k ",sizeof("SPIF erase 4k ") - 1)== 0)
	{
		*handled = TRUE;
		p = cmd + sizeof("SPIF erase 4k ") - 1;
		res = UTIL1_xatoi(&p, &val);
		if (res != ERR_OK)
		{
			CLS1_SendStr("wrong address\r\n", io->stdErr);
			return ERR_FAILED;
		}
		else
		{
			res = SPIF_EraseSector4K(val);
			if (res != ERR_OK) {
				CLS1_SendStr("failed erasing 4k sector\r\n", io->stdErr);
				return ERR_FAILED;
			}
		}
		return ERR_OK;
	} else if (UTIL1_strncmp((char* )cmd, "SPIF erase 32k ",
			sizeof("SPIF erase 32k ") - 1)
			== 0) {
		*handled = TRUE;
		p = cmd + sizeof("SPIF erase 32k ") - 1;
		res = UTIL1_xatoi(&p, &val);
		if (res != ERR_OK) {
			CLS1_SendStr("wrong address\r\n", io->stdErr);
			return ERR_FAILED;
		} else {
			res = SPIF_EraseBlock32K(val);
			if (res != ERR_OK)
			{
				CLS1_SendStr("failed erasing 32k block\r\n", io->stdErr);
				return ERR_FAILED;
			}
		}
		return ERR_OK;
	}
	else if (UTIL1_strncmp((char* )cmd, "SPIF erase 64k ",	sizeof("SPIF erase 64k ") - 1)== 0)
	{
		*handled = TRUE;
		p = cmd + sizeof("SPIF erase 64k ") - 1;
		res = UTIL1_xatoi(&p, &val);
		if (res != ERR_OK)
		{
			CLS1_SendStr("wrong address\r\n", io->stdErr);
			return ERR_FAILED;
		}
		else
		{
			res = SPIF_EraseBlock64K(val);
			if (res != ERR_OK) {
				CLS1_SendStr("failed erasing 32k block\r\n", io->stdErr);
				return ERR_FAILED;
			}
		}
		return ERR_OK;
	}
	return ERR_OK;
}

uint8_t SPIF_Init(void)
{
	  uint8_t buf[SPIF_ID_BUF_SIZE];
	  PIN_SPIF_PWR_SetVal();   //Power the Chip
	  PIN_SPIF_RESET_SetVal(); //LowActive... --> Enable Chip!
	  PIN_SPIF_WP_SetVal();	   //LowActive... --> Enable Write!

	  SPIF_CS_ENABLE();
	  SPI_WRITE(SPIF_SPI_CMD_ENABLE_4BYTE_ADD)
	  SPIF_CS_DISABLE();

	  return SPIF_ReadID(buf, sizeof(buf)); /* check ID */
}
