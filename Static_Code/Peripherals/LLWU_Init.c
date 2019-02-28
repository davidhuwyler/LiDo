/** ###################################################################
 **     Filename    : LLWU_Init.c
 **     Processor   : MK22FN512LH12
 **     Abstract    :
 **          This file implements the LLWU module initialization
 **          according to the Peripheral Initialization settings, and
 **          defines interrupt service routines prototypes.
 **
 **     Contents    :
 **         Int   - void LLWU_Init(void);
 **
 **     Copyright : 1997 - 2015 Freescale Semiconductor, Inc.
 **     All Rights Reserved.
 **
 **     Redistribution and use in source and binary forms, with or without modification,
 **     are permitted provided that the following conditions are met:
 **
 **     o Redistributions of source code must retain the above copyright notice, this list
 **       of conditions and the following disclaimer.
 **
 **     o Redistributions in binary form must reproduce the above copyright notice, this
 **       list of conditions and the following disclaimer in the documentation and/or
 **       other materials provided with the distribution.
 **
 **     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 **       contributors may be used to endorse or promote products derived from this
 **       software without specific prior written permission.
 **
 **     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 **     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 **     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 **     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 **     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 **     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 **     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 **     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 **     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 **     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **
 **     http: www.freescale.com
 **     mail: support@freescale.com
 ** ###################################################################*/

/*!
 * @file LLWU_Init.c
 * @brief This file implements the LLWU module initialization according to the
 *        Peripheral Initialization settings, and defines interrupt service
 *        routines prototypes.
 */


/* MODULE LLWU_Init. */

#include "LLWU_Init.h"
#include "MK22F51212.h"
#include "Init_Config.h"

#ifdef __cplusplus
extern "C" {
#endif

void LLWU_Init(void) {

  /* Register 'LLWU_F1' initialization */
  #ifdef LLWU_F1_VALUE
  LLWU_F1 = LLWU_F1_VALUE;
  #endif

  /* Register 'LLWU_F2' initialization */
  #ifdef LLWU_F2_VALUE
  LLWU_F2 = LLWU_F2_VALUE;
  #endif

  /* Register 'LLWU_PE1' initialization */
  #if LLWU_PE1_MASK
    #if LLWU_PE1_MASK == 0xFF
  LLWU_PE1 = LLWU_PE1_VALUE;
    #elif LLWU_PE1_MASK == LLWU_PE1_VALUE
  LLWU_PE1 |= LLWU_PE1_VALUE;
    #elif LLWU_PE1_VALUE == 0
  LLWU_PE1 &= ~LLWU_PE1_MASK;
    #else
  LLWU_PE1 = (LLWU_PE1 & (~LLWU_PE1_MASK)) | LLWU_PE1_VALUE;
    #endif
  #elif defined(LLWU_PE1_VALUE)
  LLWU_PE1 = LLWU_PE1_VALUE;
  #endif

  /* Register 'LLWU_PE2' initialization */
  #ifdef LLWU_PE2_VALUE
  LLWU_PE2 = LLWU_PE2_VALUE;
  #endif

  /* Register 'LLWU_PE3' initialization */
  #ifdef LLWU_PE3_VALUE
  LLWU_PE3 = LLWU_PE3_VALUE;
  #endif

  /* Register 'LLWU_PE4' initialization */
  #ifdef LLWU_PE4_VALUE
  LLWU_PE4 = LLWU_PE4_VALUE;
  #endif

  /* Register 'LLWU_ME' initialization */
  #if LLWU_ME_MASK
    #if LLWU_ME_MASK == 0xFF
  LLWU_ME = LLWU_ME_VALUE;
    #elif LLWU_ME_MASK == LLWU_ME_VALUE
  LLWU_ME |= LLWU_ME_VALUE;
    #elif LLWU_ME_VALUE == 0
  LLWU_ME &= ~LLWU_ME_MASK;
    #else
  LLWU_ME = (LLWU_ME & (~LLWU_ME_MASK)) | LLWU_ME_VALUE;
    #endif
  #elif defined(LLWU_ME_VALUE)
  LLWU_ME = LLWU_ME_VALUE;
  #endif

  /* Register 'LLWU_FILT1' initialization */
  #if LLWU_FILT1_MASK
    #if LLWU_FILT1_MASK == 0xFF
  LLWU_FILT1 = LLWU_FILT1_VALUE;
    #elif LLWU_FILT1_MASK == LLWU_FILT1_VALUE
  LLWU_FILT1 |= LLWU_FILT1_VALUE;
    #elif LLWU_FILT1_VALUE == 0
  LLWU_FILT1 &= ~LLWU_FILT1_MASK;
    #else
  LLWU_FILT1 = (LLWU_FILT1 & (~LLWU_FILT1_MASK)) | LLWU_FILT1_VALUE;
    #endif
  #elif defined(LLWU_FILT1_VALUE)
  LLWU_FILT1 = LLWU_FILT1_VALUE;
  #endif

  /* Register 'LLWU_FILT2' initialization */
  #if LLWU_FILT2_MASK
    #if LLWU_FILT2_MASK == 0xFF
  LLWU_FILT2 = LLWU_FILT2_VALUE;
    #elif LLWU_FILT2_MASK == LLWU_FILT2_VALUE
  LLWU_FILT2 |= LLWU_FILT2_VALUE;
    #elif LLWU_FILT2_VALUE == 0
  LLWU_FILT2 &= ~LLWU_FILT2_MASK;
    #else
  LLWU_FILT2 = (LLWU_FILT2 & (~LLWU_FILT2_MASK)) | LLWU_FILT2_VALUE;
    #endif
  #elif defined(LLWU_FILT2_VALUE)
  LLWU_FILT2 = LLWU_FILT2_VALUE;
  #endif
}

#ifdef __cplusplus
}
#endif

/* END LLWU_Init. */

/** ###################################################################
 **
 **     This file is a part of Processor Expert static initialization
 **     library for Freescale microcontrollers.
 **
 ** ################################################################### */
