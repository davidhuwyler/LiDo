/*
 * Shell.c
 *
 *  Created on: Feb 21, 2019
 *      Author: dave
 */
#include "Shell.h"
#include "CLS1.h"
#include "KIN1.h"
#include "LightSensor.h"
#include "AccelSensor.h"
#include "RTC.h"
#include "LowPower.h"
#include "SPIF.h"
#include "UTIL1.h"
#include "FileSystem.h"

static TaskHandle_t shellTaskHandle;
static TickType_t shellEnabledTimestamp;

static const CLS1_ParseCommandCallback CmdParserTable[] =
{
  CLS1_ParseCommand,
  KIN1_ParseCommand,
  LightSensor_ParseCommand,
  AccelSensor_ParseCommand,
  RTC_ParseCommand,
  SPIF_ParseCommand,
  FS_ParseCommand,
  NULL /* sentinel */
};

static void SHELL_task(void *param) {
  (void)param;
  TickType_t xLastWakeTime;
  shellEnabledTimestamp = xTaskGetTickCount();

  for(;;)
  {

	  xLastWakeTime = xTaskGetTickCount();
	  (void)CLS1_ReadAndParseWithCommandTable(CLS1_DefaultShellBuffer, sizeof(CLS1_DefaultShellBuffer), CLS1_GetStdio(), CmdParserTable);

	  if(xTaskGetTickCount() - shellEnabledTimestamp > pdMS_TO_TICKS(2000))
	  {
#if 0
		  LowPower_EnableStopMode();
		  vTaskSuspend(shellTaskHandle);
#elif 1
		  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(10));
#endif
	  }
	  else
	  {
		  vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(10));
	  }
  } /* for */
}

void SHELL_Init(void) {
  CLS1_DefaultShellBuffer[0] = '\0';
  if (xTaskCreate(SHELL_task, "Shell", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &shellTaskHandle) != pdPASS)
  {
	  for(;;){} /* error! probably out of memory */
  }
}

void SHELL_EnableShellFor20s(void)
{
	//Cpu_SetClockConfiguration((LDD_TClockConfiguration)0);
	LowPower_DisableStopMode();
	shellEnabledTimestamp = xTaskGetTickCount();
	xTaskResumeFromISR(shellTaskHandle);
	//vTaskResume(shellTaskHandle);
}

void SHELL_Disable(void)
{
	shellEnabledTimestamp = xTaskGetTickCount() - pdMS_TO_TICKS(20001);
}



/*
** ===================================================================
**     Method      :  strcatNumHex (component Utility)
**
**     Description :
**         Appends a value as hex valalue to a string buffer as hex
**         number (without a 0x prefix), with variable number of digits
**     Parameters  :
**         NAME            - DESCRIPTION
**       * dst             - Pointer to destination string
**         dstSize         - Size of the destination buffer (in
**                           bytes).
**         num             - Value to convert.
**         nofBytes        - Number of bytes to write
**     Returns     : Nothing
** ===================================================================
*/
void McuUtility_strcatNumHex(uint8_t *dst, size_t dstSize, uint32_t num, uint8_t nofBytes)
{
  if (nofBytes==1) {
      UTIL1_strcatNum8Hex(dst, dstSize, (uint8_t)num);
  } else if (nofBytes==2) {
	  UTIL1_strcatNum16Hex(dst, dstSize, (uint16_t)num);
  } else if (nofBytes==3) {
	  UTIL1_strcatNum24Hex(dst, dstSize, num);
  } else { /* nofBytes==4 */
	  UTIL1_strcatNum32Hex(dst, dstSize, num);
  }
}


/*
** ===================================================================
**     Method      :  PrintMemory (component Shell)
**
**     Description :
**         Prints a chunk of memory bytes in a formatted way.
**     Parameters  :
**         NAME            - DESCRIPTION
**       * hndl            - Pointer to
**         startAddr       - Memory start address
**         endAddr         - Memory end address
**         addrSize        - Number of bytes for the address
**                           (1, 2, 3 or 4)
**         bytesPerLine    - Number of bytes per line
**         readfp          - Function pointer to read the memory.
**                           Returns error code, uses a device handle,
**                           32bit address with a pointer to a buffer
**                           and a buffer size.
**       * io              - Pointer to I/O to be used
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t McuShell_PrintMemory(void *hndl, uint32_t startAddr, uint32_t endAddr, uint8_t addrSize, uint8_t bytesPerLine, uint8_t (*readfp)(void *, uint32_t, uint8_t*, size_t), CLS1_ConstStdIOType *io)
{
  #define NOF_BYTES_PER_LINE 32 /* how many bytes are shown on a line. This defines as well the chunk size we read from memory */
  #define MAX_NOF_BYTES_PER_LINE 32
  uint8_t buf[MAX_NOF_BYTES_PER_LINE]; /* this is the chunk of data we get (per line in output) */
  uint8_t str[3*MAX_NOF_BYTES_PER_LINE+((MAX_NOF_BYTES_PER_LINE+1)/8)+1]; /* maximum string for output:
                                              - '3*' because each byte is 2 hex digits plus a space
                                              - '(NOF_BYTES_PER_LINE+1)/8' because we add a space between every 8 byte block
                                              - '+1' for the final zero byte */
  uint32_t addr;
  uint8_t res=0, j, bufSize;
  uint8_t ch;

  if (endAddr<startAddr) {
    CLS1_SendStr((unsigned char*)"\r\n*** End address must be larger or equal than start address\r\n", io->stdErr);
    return ERR_RANGE;
  }
  for(addr=startAddr; addr<=endAddr; /* nothing */ ) {
    if (endAddr-addr+1 >= bytesPerLine) { /* read only part of buffer */
      bufSize = bytesPerLine; /* read full buffer */
    } else {
      bufSize = (uint8_t)(endAddr-addr+1);
    }
    if (readfp(hndl, addr, buf, bufSize)!=ERR_OK) {
    	CLS1_SendStr((unsigned char*)"\r\n*** Read failed!\r\n", io->stdErr);
      return ERR_FAILED;
    }
    if (res != ERR_OK) {
    	CLS1_SendStr((unsigned char*)"\r\n*** Failure reading memory block!\r\n", io->stdErr);
      return ERR_FAULT;
    }
    /* write address */
    UTIL1_strcpy(str, sizeof(str), (unsigned char*)"0x");
    McuUtility_strcatNumHex(str, sizeof(str), addr, addrSize);
    UTIL1_chcat(str, sizeof(str), ':');
    CLS1_SendStr((unsigned char*)str, io->stdOut);
    /* write data in hex */
    str[0] = '\0';
    for (j=0; j<bufSize; j++) {
      if ((j)==0) {
    	  UTIL1_chcat(str, sizeof(str), ' ');
      }
      UTIL1_strcatNum8Hex(str, sizeof(str), buf[j]);
      UTIL1_chcat(str, sizeof(str), ' ');
    }
    for (/*empty*/; j<bytesPerLine; j++) { /* fill up line */
    	UTIL1_strcat(str, sizeof(str), (unsigned char*)"-- ");
    }
    CLS1_SendStr((unsigned char*)str, io->stdOut);
    /* write in ASCII */
    io->stdOut(' ');
    for (j=0; j<bufSize; j++) {
      ch = buf[j];
      if (ch >= ' ' && ch <= 0x7f) {
        io->stdOut(ch);
      } else {
        io->stdOut('.'); /* place holder */
      }
    }
    for (/*empty*/; j<bytesPerLine; j++) { /* fill up line */
    	UTIL1_strcat(str, sizeof(str), (unsigned char*)"-- ");
    }
    CLS1_SendStr((unsigned char*)"\r\n", io->stdOut);
    addr += bytesPerLine;
  }
  return ERR_OK;
}




