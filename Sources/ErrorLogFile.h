/*
 * ErrorLogFile.h
 *
 *  Created on: 25.03.2019
 *      Author: dhuwiler
 */

#ifndef SOURCES_ERRORLOGFILE_H_
#define SOURCES_ERRORLOGFILE_H_

#include "Platform.h"
#include "CLS1.h"

uint8_t ErrorLogFile_Init(void);
uint8_t ErrorLogFile_LogError(uint16 SDEP_error_alert_cmdId, uint8_t* message);
uint8_t ErrorLogFile_ParseCommand(const uint8_t *cmd, bool *handled, const CLS1_StdIOType *io);


#endif /* SOURCES_ERRORLOGFILE_H_ */
