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

void SHELL_Parse(void);
void SHELL_Init(void);

CLS1_ConstStdIOType *SHELL_GetStdio(void);
void SHELL_requestDisabling(void);

#endif /* SOURCES_SHELL_H_ */
