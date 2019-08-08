/*
 * FileSystem.h
 *
 *  Created on: Mar 1, 2019
 *      Author: dave
 */

#ifndef SOURCES_FILESYSTEM_H_
#define SOURCES_FILESYSTEM_H_

#include "Platform.h"
#include "CLS1.h"
#include "littleFS/lfs.h"
#include "Application.h"
#include "FRTOS1.h"

bool FS_IsMounted(void);

lfs_t* FS_GetFileSystem(void);
uint8_t FS_ParseCommand(const unsigned char* cmd, bool *handled, const CLS1_StdIOType *io);
uint8_t FS_Init(void);
uint8_t FS_FormatInit(void);
void FS_GetFileAccessSemaphore(SemaphoreHandle_t* sema);

uint8_t FS_ReadFile(lfs_file_t* file, bool readFromBeginning, size_t nofBytes, CLS1_ConstStdIOType *io);
uint8_t FS_FileList(const char *path, CLS1_ConstStdIOType *io);
uint8_t FS_RemoveFile(const char *filePath, CLS1_ConstStdIOType *io);
uint8_t FS_MoveFile(const char *srcPath, const char *dstPath,CLS1_ConstStdIOType *io);

uint8_t FS_openFile(lfs_file_t* file,uint8_t* filename);
uint8_t FS_closeFile(lfs_file_t* file);
uint8_t FS_writeLine(lfs_file_t* file,uint8_t* line);
uint8_t FS_readLine(lfs_file_t* file,uint8_t* lineBuf,size_t bufSize,uint8_t* nofReadChars);

//LiDo Specific Functions
uint8_t FS_openLiDoSampleFile(lfs_file_t* file);
uint8_t FS_writeLiDoSample(liDoSample_t *sample,lfs_file_t* file);
uint8_t FS_getLiDoSampleOutOfFile(lfs_file_t* file,uint8_t* sampleBuf,size_t bufSize,uint8_t* nofReadChars);

//Functions ported from FatFS (Used by MiniIni)
char* FS_gets (char* buff,int len, lfs_file_t* fp);
int FS_putc (char c, lfs_file_t* fp);
int FS_puts (const char* str, lfs_file_t* fp);

#endif /* SOURCES_FILESYSTEM_H_ */
