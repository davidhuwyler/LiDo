/*
 * AppDataFile.h
 *
 *  Created on: Mar 9, 2019
 *      Author: dave
 */

#ifndef SOURCES_APPDATAFILE_H_
#define SOURCES_APPDATAFILE_H_

/* The AppDataFile is stores App Data on the SPIF.
 * The Data is Stored in the appDataFile.txt file.
 *
 * The Data is organized in lines:
 *
 * <Key> <size> <ValueString>\0\r\n
 *
 * Ex:
 *
 * \r\n
 * 0 6 LiDo01\0\r\n
 *...
 *
 */

typedef enum
{
	AppDataFileKey_Name,
	AppDataFileKey_ID,
	AppDataFileKey_Version,
	AppDataFileKey_SampleIntervall,
	AppDataFileKey_NumberOfKeys
} AppDataFileKey_e;

uint8_t AppDataFile_Init(void);
uint8_t AppDataFile_Sync(void);
uint8_t AppDataFile_Get(AppDataFileKey_e key,uint8_t* size,uint8_t* value);



#endif /* SOURCES_APPDATAFILE_H_ */
