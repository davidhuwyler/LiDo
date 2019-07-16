/*
 * SPIF.h
 *
 *  Created on: Mar 1, 2019
 *      Author: dave
 */

#ifndef SOURCES_SPIF_H_
#define SOURCES_SPIF_H_

#include "Platform.h"
#include <stddef.h>

#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
  uint8_t SPIF_ParseCommand(const unsigned char* cmd, bool *handled, const CLS1_StdIOType *io);
#endif

void SPIF_PowerOn(void);
void SPIF_PowerOff(void);


#define SPIF_ID_BUF_SIZE  (3)

void SPIF_SPI_BlockReceived(void);

uint8_t SPIF_ReadID(uint8_t *buf, size_t bufSize);

uint8_t SPIF5_ReadStatus(uint8_t *status);

bool SPIF_isBusy(void);

void SPIF_WaitIfBusy(void);

uint8_t SPIF_Read(uint32_t address, uint8_t *buf, size_t bufSize);

uint8_t SPIF_EraseAll(void);

uint8_t SPIF_EraseSector4K(uint32_t address);

uint8_t SPIF_EraseBlock32K(uint32_t address);

uint8_t SPIF_EraseBlock64K(uint32_t address);

uint8_t SPIF_GoIntoDeepPowerDown();

uint8_t SPIF_ReleaseFromDeepPowerDown();

/*!
 * Program a page with data
 * \param address, should be aligned on page (256 bytes) if programming 256 bytes
 * \param data pointer to data
 * \param dataSize size of data in bytes, max 256
 * \return error code, ERR_OK for no error
 */
uint8_t SPIF_ProgramPage(uint32_t address, const uint8_t *data, size_t dataSize);

uint8_t SPIF_GetCapacity(const uint8_t *id, uint32_t *capacity);

uint8_t SPIF_Init(void);

#endif /* SOURCES_SPIF_H_ */
