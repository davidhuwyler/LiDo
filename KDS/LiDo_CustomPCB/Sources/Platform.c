/*
 * Platform.c
 *
 *  Created on: 14.07.2019
 *      Author: Erich Styger Local
 */

#include "Platform.h"
#include "Application.h"
#include "AppDataFile.h"
#include "WatchDog.h"
#include "PowerManagement.h"
#include "LightSensor.h"
#include "AccelSensor.h"
#include "LowPower.h"
#include "McuLC709203F.h"
#include "Shell.h"
#include "SPIF.h"
#include "SDEP.h"
#include "FileSystem.h"
#include "RTC.h"
#include "UI.h"
#include "I2C_SCL.h"
#include "I2C_SDA.h"
#include "PORT_PDD.h"
#include "GPIO_PDD.h"
#include "WAIT1.h"
#include "PORT_PDD.h"
#include "RES_OPT.h"
#include "INT_LI_DONE.h"
#include "PIN_PS_MODE.h"
#include "Shell.h"

static void MuxAsGPIO(void) {
  /* PTB3: SDA, PTB2: SCL */
  //CI2C1_Deinit(NULL);
  I2C_SDA_ConnectPin(); /* mux as GPIO */
  I2C_SCL_ConnectPin(); /* mux as GPOO */
  I2C_SDA_SetOutput();
  I2C_SCL_SetOutput();
}

static void MuxAsI2C(void) {
  /* PTB3: SDA, PTB2: SCL */
  //CI2C1_Init(NULL);
  /* mux back to normal I2C mode with interrupts enabled */
  /* PCR3: SDA */
  PORTB_PCR3 = (uint32_t)((PORTB_PCR3 & (uint32_t)~(uint32_t)(
                PORT_PCR_ISF_MASK |
                PORT_PCR_MUX(0x05)
               )) | (uint32_t)(
                PORT_PCR_MUX(0x02)
               ));
  PORT_PDD_SetPinOpenDrain(PORTB_BASE_PTR, 0x03u, PORT_PDD_OPEN_DRAIN_ENABLE); /* Set SDA pin as open drain */
  /* PORTB_PCR2: ISF=0,MUX=2 */
  PORTB_PCR2 = (uint32_t)((PORTB_PCR2 & (uint32_t)~(uint32_t)(
                PORT_PCR_ISF_MASK |
                PORT_PCR_MUX(0x05)
               )) | (uint32_t)(
                PORT_PCR_MUX(0x02)
               ));
  PORT_PDD_SetPinOpenDrain(PORTB_BASE_PTR, 0x02u, PORT_PDD_OPEN_DRAIN_ENABLE); /* Set SCL pin as open drain */
}

static void ResetI2CBus(void) {
  int i;
  MuxAsGPIO();

  /* Drive SDA low first to simulate a start */
  I2C_SDA_ClrVal();//McuGPIO_Low(sdaPin);
  WAIT1_Waitus(10);
  /* Send 9 pulses on SCL and keep SDA high */
  for (i = 0; i < 9; i++) {
    I2C_SCL_ClrVal();//McuGPIO_Low(sclPin);
    WAIT1_Waitus(10);

    I2C_SDA_SetVal();//McuGPIO_High(sdaPin);
    WAIT1_Waitus(10);

    I2C_SCL_SetVal();//McuGPIO_High(sclPin);
    WAIT1_Waitus(20);
  }
  /* Send stop */
  I2C_SCL_ClrVal();//McuGPIO_Low(sclPin);
  WAIT1_Waitus(10);
  I2C_SDA_ClrVal();//McuGPIO_Low(sdaPin);
  WAIT1_Waitus(10);

  I2C_SCL_ClrVal();//McuGPIO_Low(sclPin);
  WAIT1_Waitus(10);

  I2C_SDA_SetVal();//McuGPIO_High(sdaPin);
  WAIT1_Waitus(10);
  /* go back to I2C mode */
  MuxAsI2C();
}

uint8_t PL_InitWithInterrupts(void) {
  /* function gets called from a task where interrupts are enabled */
  uint8_t res = ERR_OK;

  WatchDog_Init();
  WatchDog_StartComputationTime(WatchDog_LiDoInit);
  res = FS_Init(); /* calls SPIF_Init() too!  SPI Flash chip needs to be initialized, otherwise it drains around 800uA! */
  if (res!=ERR_OK) {
    CLS1_SendStr("FAILED to initialize file system!\r\n", SHELL_GetStdio()->stdErr);
    /* continue execution, but indicate error with red LED blinking in main application loop */
  }
  LightSensor_init(); /* uses I2C */
#if PL_CONFIG_HAS_ACCEL_SENSOR /* Accelerometer uses I2C */
  AccelSensor_Init();
#if 0
  bool isEnabled;

  res = AccelSensor_DisableTemperatureSensor();
  if (res!=ERR_OK) {
    APP_FatalError(__FILE__, __LINE__);
  }
  res = AccelSensor_SetPowerMode(LIS2DH_CTRL_REG1_POWERMODE_POWERDOWN);
  if (res!=ERR_OK) {
    APP_FatalError(__FILE__, __LINE__);
  }
#endif
#endif
#if PL_CONFIG_HAS_GAUGE_SENSOR
  McuLC_Init(); /* initialize gauge sensor, needs I2C interrupts */
#endif
  AppDataFile_Init(); /* reads from file system, uses SPI interrupts */
  if(RCM_SRS0 & RCM_SRS0_POR_MASK) { // Init from PowerOn Reset
    AppDataFile_SetStringValue(APPDATA_KEYS_AND_DEV_VALUES[4][0], "0"); //Disable Sampling
    RTC_Init(FALSE);   /* HardReset RTC */
  } else {
    RTC_Init(TRUE); /* soft-reset RTC */
  }
  WatchDog_StopComputationTime(WatchDog_LiDoInit);
  return res; /* return error code (if any) */
}

void PL_Init(void) {
  /* because interrupts are disabled, only initialize things which do not require interrupts, so no I2C and SPI */
  PowerManagement_PowerOn(); /* turn on FET to keep Vcc supplied. should already be done by default during PE_low_level_init() */
  LowPower_Init();
  /* ---------------------------------------------------------------*/
  //PIN_PS_MODE_SetVal(); /* disable low power DC-DC (low active) */
  PIN_PS_MODE_ClrVal(); /* enable low power DC-DC */
  /* ---------------------------------------------------------------*/
  /* Light Sensor */
  /* pull-down for INT_LI_DONE */
  INT_LI_DONE_Disable(); /* disable interrupt */
#if PL_BOARD_REV==20 || PL_BOARD_REV==21 /* PTB0 */
  PORT_PDD_SetPinPullSelect(PORTB_BASE_PTR, 0, PORT_PDD_PULL_DOWN);
  PORT_PDD_SetPinPullEnable(PORTB_BASE_PTR, 0, PORT_PDD_PULL_ENABLE);
#elif PL_BOARD_REV==22 /* PTA4 */
  PORT_PDD_SetPinPullSelect(PORTA_BASE_PTR, 4, PORT_PDD_PULL_DOWN);
  PORT_PDD_SetPinPullEnable(PORTA_BASE_PTR, 4, PORT_PDD_PULL_ENABLE);
#endif
#if PL_BOARD_REV==22
  RES_OPT_SetVal(); /* disable reset on AS 7264 */
  RES_OPT_ClrVal(); /* reset AS7264 */
#endif

#if PL_CONFIG_HAS_GAUGE_SENSOR
  McuLC_Wakeup(); /* needs to be done before (!!!) any other I2C communication! */
#endif
  ResetI2CBus();
#if PL_CONFIG_HAS_GAUGE_SENSOR
  McuLC_Wakeup(); /* need to do the wake-up again after the reset bus? otherwise will be stuck in McuLC_Init() */
#endif
#if PL_CONFIG_HAS_SHELL
  SHELL_Init();
#endif
  APP_Init();
  UI_Init();
  PowerManagement_init();
  SDEP_Init();

}

