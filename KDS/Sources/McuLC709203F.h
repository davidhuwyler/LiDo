/*
 * Copyright (c) 2019, Erich Styger
 * All rights reserved.
 *
 * Interface for for the LC709203 battery gauge I2C sensor
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SOURCES_MCULC709203F_H_
#define SOURCES_MCULC709203F_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Returns the current battery voltage
 * \param pVoltage Pointer where to store the value (in mV)
 * \return Error code, ERR_OK for no error
 */
uint8_t McuLC_GetVoltage(uint16_t *pVoltage);

uint8_t McuLC_GetTemperature(int16_t *pTemperature);
uint8_t McuLC_GetRSOC(uint16_t *pRsoc);
uint8_t McuLC_GetITE(uint16_t *pIte);
uint8_t McuLC_GetICversion(uint16_t *pVersion);

uint8_t McuLC_SetTemperatureMeasurementMode(bool i2cMode);

uint8_t McuLC_SetPowerMode(bool sleepMode);

/*!
 * \brief Get the current temperature measurement mode
 * \param pDir Pointer where to store the mode
 * \return Error code, ERR_OK if everything is ok
 */
uint8_t McuLC_GetTemperatureMeasurementMode(bool *isI2Cmode);

typedef enum {
  McuLC_CURRENT_DIR_AUTO,         /*!< automatic mode (default) */
  McuLC_CURRENT_DIR_CHARGING,     /*!< charging mode */
  McuLC_CURRENT_DIR_DISCHARING,   /*!< discharging mode */
  McuLC_CURRENT_DIR_ERROR         /*!< error case */
} McuLC_CurrentDirection;
/*!< Current direction used for McuLC_GetCurrentDirection() and McuLC_SetCurrentDirection() */

/*!
 * \brief Get the current direction mode
 * \param pDir Pointer where to store the mode
 * \return Error code, ERR_OK if everything is ok
 */
uint8_t McuLC_GetCurrentDirection(McuLC_CurrentDirection *pDir);

/*!
 * \brief Set the current direction mode
 * \param dir Mode to set
 * \return Error code, ERR_OK if everything is ok
 */
uint8_t McuLC_SetCurrentDirection(McuLC_CurrentDirection dir);

#include "CLS1.h"
uint8_t McuLC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

/*!
 * \brief Wake up the device from sleep mode. Uses bit-banging. If other devices are on the bus, this method has to be called before anything else!
 */
void McuLC_Wakeup(void); /* must be done before any other I2C communication on the bus! */

/*!
 * \brief Driver initialization. I2C bus must be operational for this.
 * \return Error code, ERR_OK if everything is ok
 */
uint8_t McuLC_Init(void);

/*!
 * \brief Driver de-initialisation
 */
void McuLC_Deinit(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* SOURCES_MCULC709203F_H_ */
