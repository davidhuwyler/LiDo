/*
 * SDEPshellHandler.h
 *
 *  Created on: Mar 8, 2019
 *      Author: dave
 */

#ifndef SOURCES_SDEPSHELLHANDLER_H_
#define SOURCES_SDEPSHELLHANDLER_H_

#include <stdint.h>
#include <stdbool.h>

void SDEPshellHandler_init(void);

uint8_t SDEP_HandleShellCMDs(void);
uint8_t SDEP_SendData(const uint8_t *data, uint8_t size);
uint8_t SDEP_SendChar(uint8_t c);
uint8_t SDEP_ReadChar(uint8_t *c);
uint8_t SDEP_SDEPtoShell(const uint8_t *str,uint8_t size);
uint8_t SDEP_ShellToSDEP(const uint8_t*str,uint8_t* size);
void SDEPshellHandler_switchIOtoSDEPio(void);
void SDEPshellHandler_switchIOtoStdIO(void);
bool SDEPshellHandler_NewSDEPmessageAvail(void);

//SDEP StdIO functions:
void SDEP_ShellReadChar(uint8_t *c);
void SDEP_ShellSendChar(uint8_t ch);
bool SDEP_ShellKeyPressed(void);


#endif /* SOURCES_SDEPSHELLHANDLER_H_ */
