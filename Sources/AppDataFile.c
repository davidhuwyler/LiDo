/*
 * AppDataFile.c
 *
 *  Created on: Mar 9, 2019
 *      Author: dave
 */

#include "FileSystem.h"
#include "littleFS/lfs.h"
#include "AppDataFile.h"

static lfs_t* FS_lfs;
static lfs_file_t file;

#define FILE_BUF_SIZE 50


static uint8_t readFromFile(uint8_t *buf,size_t bufSize)
{
	if (lfs_file_read(FS_lfs, &file, buf, bufSize) < 0)
	{
		return ERR_FAILED;
	}
	return ERR_OK;
}


static uint8_t AppDataFile_FindLineInBuf(uint8_t *buf,size_t bufSize,uint8_t* startIndex,uint8_t* stopIndex)
{
	bool foundStart = false;
	bool foundStop = false;
	for(int i = 0 ; i<bufSize-1 ; i++)
	{
		if(buf[i] == '\r' && buf[i+1] == '\n' && !foundStart)
		{
			if(!foundStart)
			{
				*startIndex =i+2;
				foundStart = true;
			}
			else
			{
				*stopIndex =i-1;
				foundStart = true;
				return ERR_OK;
			}
		}
	}
	return ERR_FAILED;
}

uint8_t AppDataFile_Get(AppDataFileKey_e key,uint8_t* size,uint8_t* value)
{
	uint32_t fileSize;
	uint8_t buf[FILE_BUF_SIZE];
	uint8_t* p;
	uint8_t res;
	bool endOfFile = false;

	uint8_t lineStartIndex = 0, lineStopIndex = 0;


	for(int i = 0 ; i < AppDataFileKey_NumberOfKeys ; i++)
	{
		uint8_t currentKey;
		//ReadFromFile
		fileSize = lfs_file_size(FS_lfs, &file);
		if(fileSize<=0)
		{
			return ERR_FAILED;
		}
		if(fileSize < (FILE_BUF_SIZE-lineStartIndex))
		{
			if(readFromFile(buf+lineStartIndex,(size_t)fileSize)!=ERR_OK ||
			   AppDataFile_FindLineInBuf(buf,(size_t)fileSize,&lineStartIndex, &lineStopIndex) != ERR_OK)
			{
				return ERR_FAILED;
			}

			endOfFile = true;
		}
		else
		{
			if(readFromFile(buf+lineStartIndex,(size_t)(FILE_BUF_SIZE-lineStartIndex))!=ERR_OK ||
			   AppDataFile_FindLineInBuf(buf+lineStartIndex,(size_t)(FILE_BUF_SIZE-lineStartIndex),&lineStartIndex, &lineStopIndex) != ERR_OK)
			{
				return ERR_FAILED;
			}
		}

		//Read
		p = buf + lineStartIndex;
		res = UTIL1_xatoi((const unsigned char **)&p, (int32_t*)&currentKey);
		if (res != ERR_OK)
		{
			return ERR_FAILED;
		}

		if(currentKey == (uint8_t)key)
		{
			res = UTIL1_xatoi((const unsigned char **)&p,  (int32_t*)size);
			if (res != ERR_OK)
			{
				return ERR_FAILED;
			}
			value = p;
			return ERR_OK;
		}

		for(int i = 0 ; i < (FILE_BUF_SIZE-lineStartIndex) ; i++)
		{
			*(buf+i) = *(buf+1+lineStartIndex);
		}
	}
}

static uint8_t AppDataFile_WriteWholeFile(void)
{
	uint8_t buf[FILE_BUF_SIZE];

	for (int i = 0; i < AppDataFileKey_NumberOfKeys ; i++)
	{
		UTIL1_Num8uToStr(buf,FILE_BUF_SIZE,i);
		UTIL1_chcat(buf,FILE_BUF_SIZE,' ');
		UTIL1_Num8uToStr(buf,FILE_BUF_SIZE,0);
		UTIL1_strcat(buf,FILE_BUF_SIZE," \r\n");
		if (lfs_file_write(FS_lfs, &file,buf,UTIL1_strlen(buf)) < 0)
		{
			lfs_file_close(FS_lfs, &file);
			return ERR_FAILED;
		}
	}
	lfs_file_close(FS_lfs, &file);
	return ERR_OK;
}



uint8_t AppDataFile_Init(void)
{
	FS_lfs = FS_GetFileSystem();

	if (lfs_file_open(FS_lfs, &file, "./appDataFile.txt", LFS_O_RDWR | LFS_O_EXCL)< 0)
	{
		// !? lfs_file_close(FS_lfs, &file);
		//FileDosnt Exist jet
		if (lfs_file_open(FS_lfs, &file, "./appDataFile.txt", LFS_O_RDWR | LFS_O_CREAT)< 0)
		{
			AppDataFile_WriteWholeFile();

		}
	}
}
