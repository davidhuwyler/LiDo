/*
 * Shell.h
 *
 *  Created on: Feb 21, 2019
 *      Author: dave
 */

#ifndef SOURCES_SHELL_H_
#define SOURCES_SHELL_H_

//#include <stdint.h>
//#include <stddef.h>

#include "Platform.h"
#include "CLS1.h"

void SHELL_Parse(void);
void SHELL_Init(void);

void SHELL_EnableShellFor20s(void);
void SHELL_requestDisabling(void);

uint8_t McuShell_PrintMemory(void *hndl, uint32_t startAddr, uint32_t endAddr, uint8_t addrSize, uint8_t bytesPerLine, uint8_t (*readfp)(void *, uint32_t, uint8_t*, size_t), CLS1_ConstStdIOType *io);


#endif /* SOURCES_SHELL_H_ */
