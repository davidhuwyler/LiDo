/*
 * FileSystem.h
 *
 *  Created on: Mar 1, 2019
 *      Author: dave
 */

#ifndef SOURCES_FILESYSTEM_H_
#define SOURCES_FILESYSTEM_H_

#include <stdint.h>
#include <stdbool.h>
#include "CLS1.h"
#include "littleFS/lfs.h"

lfs_t* FS_GetFileSystem(void);
uint8_t FS_ParseCommand(const unsigned char* cmd, bool *handled, const CLS1_StdIOType *io);
uint8_t FS_Init(void);



#endif /* SOURCES_FILESYSTEM_H_ */
