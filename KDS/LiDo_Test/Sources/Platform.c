/*
 * Platform.c
 *
 *  Created on: 14.07.2019
 *      Author: Erich Styger Local
 */

#include "Platform.h"
#include "PowerManagement.h"
#include "LightSensor.h"
#include "McuLC709203F.h"
#include "I2C_SCL.h"
#include "I2C_SDA.h"
#include "PORT_PDD.h"
#include "GPIO_PDD.h"
#include "WAIT1.h"

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

void PL_Init(void) {
  //PowerManagement_PowerOn(); /* turn on FET to keep Vcc supplied. should already be done by default during PE_low_level_init() */
  //LightSensor_init();

#if PL_CONFIG_HAS_GAUGE_SENSOR
    McuLC_Wakeup(); /* needs to be done before (!!!) any other I2C communication! */
#endif
    ResetI2CBus();
#if PL_CONFIG_HAS_GAUGE_SENSOR
    McuLC_Wakeup(); /* need to do the wake-up again after the reset bus? otherwise will be stuck in McuLC_Init() */
#endif
}

