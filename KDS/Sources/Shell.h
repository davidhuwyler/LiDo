/*
 * Shell.h
 *
 *  Created on: Feb 21, 2019
 *      Author: dave
 */

#ifndef SOURCES_SHELL_H_
#define SOURCES_SHELL_H_

#include "Platform.h"
#include "CLS1.h"

#define SHELL_MIN_ENABLE_TIME_AFTER_BOOT_MS 10000  /* after this milli seconds, the shell gets disabled */

void SHELL_Parse(void);
void SHELL_Init(void);

void SHELL_requestDisabling(void);

#endif /* SOURCES_SHELL_H_ */
