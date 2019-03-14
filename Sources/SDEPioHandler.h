/*
 * SDEPshellHandler.h
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 */

#ifndef SOURCES_SDEPIOHANDLER_H_
#define SOURCES_SDEPIOHANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "CLS1.h"

void SDEPio_init(void);

uint8_t SDEPio_HandleShellCMDs(void);
uint8_t SDEPio_HandleFileCMDs(uint16_t cmdId);
uint8_t SDEPio_SendData(const uint8_t *data, uint8_t size);
uint8_t SDEPio_SendChar(uint8_t c);
uint8_t SDEPio_ReadChar(uint8_t *c);
uint8_t SDEPio_SDEPtoShell(const uint8_t *str,uint8_t size);
uint8_t SDEPio_ShellToSDEP(const uint8_t*str,uint8_t* size);
void SDEPio_switchIOtoSDEPio(void);
void SDEPio_switchIOtoStdIO(void);
bool SDEPio_NewSDEPmessageAvail(void);
uint8_t SDEPio_SetReadFileCMD(uint8_t* filename);

//SDEP Shell io functions:
void SDEPio_ShellReadChar(uint8_t *c);
void SDEPio_ShellSendChar(uint8_t ch);
bool SDEPio_ShellKeyPressed(void);

//SDEP File io
void SDEPio_FileReadChar(uint8_t *c);
void SDEPio_FileSendChar(uint8_t ch);
bool SDEPio_FileKeyPressed(void);
void SDEPio_getSDEPfileIO(CLS1_ConstStdIOTypePtr* io);

#endif /* SOURCES_SDEPIOHANDLER_H_ */
