/*
 * McuLC709203Fconfig.h
 *
 *  Created on: 19.06.2019
 *      Author: Erich Styger
 */

#ifndef SOURCES_MCULC709203FCONFIG_H_
#define SOURCES_MCULC709203FCONFIG_H_

#ifndef MCULC709203F_CONFIG_USE_THERMISTOR
  #define MCULC709203F_CONFIG_USE_THERMISTOR  (0)
    /*!< 1: using Thermistor in battery. 0: using I2C mode */
#endif

#ifndef MCULC_CONFIG_BLOCK_ON_I2C_ERROR
  #define MCULC_CONFIG_BLOCK_ON_I2C_ERROR   (1) /* if it should block on error and wait for the watchdog */
#endif

#endif /* SOURCES_MCULC709203FCONFIG_H_ */
