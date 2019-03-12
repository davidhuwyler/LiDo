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

uint8_t FS_RemoveFile(const char *filePath, CLS1_ConstStdIOType *io);
uint8_t FS_MoveFile(const char *srcPath, const char *dstPath,CLS1_ConstStdIOType *io);


//Functions ported from FatFS
char* FS_gets ( char* buff,int len, lfs_file_t* fp);
int FS_putc (char c, lfs_file_t* fp);
int FS_puts (const char* str, lfs_file_t* fp);

#endif /* SOURCES_FILESYSTEM_H_ */
