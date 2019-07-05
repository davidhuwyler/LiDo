/*
 * Copyright (c) 2019, Erich Styger
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* header file is included with -include compiler option */

#ifndef INCLUDEMCULIBCONFIG_H_
#define INCLUDEMCULIBCONFIG_H_

#define McuLib_CONFIG_SDK_VERSION_USED  McuLib_CONFIG_SDK_MCUXPRESSO_2_0
#define McuLib_CONFIG_CPU_IS_KINETIS    (1)
#define McuLib_CONFIG_FPU_PRESENT       (0) /* no FPU on K22DX256 */

/* ------------------- RTOS ---------------------------*/
#define configTOTAL_HEAP_SIZE                 (16*1024)
#define McuLib_CONFIG_SDK_USE_FREERTOS        (1)
#define configUSE_SEGGER_SYSTEM_VIEWER_HOOKS  (0)  /* \todo */

/* ------------------- I2C ---------------------------*/
#if 0
#define CONFIG_USE_HW_I2C                             (0) /* if using HW I2C, otherwise use software bit banging */
#define McuGenericI2C_CONFIG_USE_ON_ERROR_EVENT       (0)
#define McuGenericI2C_CONFIG_USE_ON_REQUEST_BUS_EVENT (0)
#define McuGenericI2C_CONFIG_USE_MUTEX                (0 && McuLib_CONFIG_SDK_USE_FREERTOS)

/* I2C pins */
#define I2CLIB_SCL_GPIO         GPIO
#define I2CLIB_SCL_GPIO_PORT    1
#define I2CLIB_SCL_GPIO_PIN     20
#define I2CLIB_SDA_GPIO         GPIO
#define I2CLIB_SDA_GPIO_PORT    1
#define I2CLIB_SDA_GPIO_PIN     21

#if CONFIG_USE_HW_I2C /* implementation in i2clib.c */
  #define McuGenericI2C_CONFIG_INTERFACE_HEADER_FILE            "i2clib.h"
  #define McuGenericI2C_CONFIG_RECV_BLOCK                       I2CLIB_RecvBlock
  #define McuGenericI2C_CONFIG_SEND_BLOCK                       I2CLIB_SendBlock
  #if McuGenericI2C_CONFIG_SUPPORT_STOP_NO_START
  #define McuGenericI2C_CONFIG_SEND_BLOCK_CONTINUE              I2CLIB_SendBlockContinue
  #endif
  #define McuGenericI2C_CONFIG_SEND_STOP                        I2CLIB_SendStop
  #define McuGenericI2C_CONFIG_SELECT_SLAVE                     I2CLIB_SelectSlave
  #define McuGenericI2C_CONFIG_RECV_BLOCK_CUSTOM_AVAILABLE      (0)
  #define McuGenericI2C_CONFIG_RECV_BLOCK_CUSTOM                I2CLIB_RecvBlockCustom
#else
  /* settings for GenericSWI2C */
  #define SCL1_CONFIG_GPIO_NAME     I2CLIB_SCL_GPIO
  #define SCL1_CONFIG_PORT_NAME     I2CLIB_SCL_GPIO_PORT
  #define SCL1_CONFIG_PIN_NUMBER    I2CLIB_SCL_GPIO_PIN

  #define SDA1_CONFIG_GPIO_NAME     I2CLIB_SDA_GPIO
  #define SDA1_CONFIG_PORT_NAME     I2CLIB_SDA_GPIO_PORT
  #define SDA1_CONFIG_PIN_NUMBER    I2CLIB_SDA_GPIO_PIN

  /* I2C Pin Muxing */
  #define SDA1_CONFIG_DO_PIN_MUXING (1)
  #define SCL1_CONFIG_DO_PIN_MUXING (1)

  #define McuGenericSWI2C_CONFIG_DO_YIELD (0 && McuLib_CONFIG_SDK_USE_FREERTOS) /* because of Yield in GenericSWI2C */
  #define McuGenericSWI2C_CONFIG_DELAY_NS (0)
#endif
#endif
/* -------------------------------------------------*/
/* Shell */
#define McuShell_CONFIG_PROJECT_NAME_STRING "LiDo on NXP K22DX256"
/* -------------------------------------------------*/
#if 0
/* LittlevGL */
#define LV_CONFIG_DISPLAY_WIDTH        (240)
#define LV_CONFIG_DISPLAY_HEIGHT       (320)
#define LV_CONFIG_COLOR_DEPTH          (16)
#define LV_CONFIG_DPI                  (50)
#define LV_CONFIG_COLOR_16_SWAP        (1)
#endif

#endif /* INCLUDEMCULIBCONFIG_H_ */

