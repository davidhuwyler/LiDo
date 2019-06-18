/*
 * LC709203.c
 *
 *  Created on: 06.06.2019
 *      Author: Sarah Gander
 */
#include "Platform.h"
#if PL_CONFIG_HAS_GAUGE_SENSOR
#include "LC709203F.h"
#include "platform.h"
#include "GI2C1.h"
#include "UTIL1.h"
#include "WAIT1.h"
#include "LED_R.h"

#define LC709203F_I2C_SLAVE_ADDR  0x0B

#define LC709203F_REG_SET_BEFORE_RSOC   0x04  /* Before RSCOC */
#define LC709203F_REG_SET_THERMB        0x06  /* Thermitor B */
#define LC709203F_REG_INIT_RSOC         0x07  /* Initial RSOC */
#define LC709203F_REG_CELL_TEMP         0x08  /* Cell temperature */
#define LC709203F_REG_VOLTAGE           0x09  /* Cell voltage */
#define LC709203F_REG_ADJ_APPLI         0x0B  /* Current direction */
#define LC709203F_REG_RSOC              0x0D  /* RSOC, value based 0-100 scale */
#define LC709203F_REG_ITE               0x0F  /* ITE, Indicator to Empty */
#define LC709203F_REG_IC_VER            0x11  /* IC version */
#define LC709203F_REG_CHG_PARAM         0x12  /* change of parameter */
#define LC709203F_REG_RSOC_ALM          0x13  /* Alarm low RSOC */
#define LC709203F_REG_LOW_CELL_VOL_ALM  0x14  /* Alarm Low Cell Voltage */
#define lC709203F_REG_PW_MODE           0x15  /* IC Power Mode */
#define LC709203F_REG_EN_NTC            0x16  /* Status Bit, enable Termistor mode */
#define LC709203F_REG_NUM_PARAMS        0x1A  /* Number of the parameter, display batter profile code */

static uint8_t i2cReadCmdData(uint8_t i2cAddr, uint8_t cmd, uint8_t *data, size_t length) {
  return GI2C1_ReadAddress(i2cAddr, &cmd, sizeof(cmd), data, length);
}

static uint8_t i2cWriteCmdData(uint8_t i2cAddr, uint8_t cmd, uint8_t *data, size_t length) {
  return GI2C1_WriteAddress(i2cAddr, &cmd, sizeof(cmd), data, length);
}

static uint8_t calcCrc8Atm(uint8_t a, uint8_t b) {
  /* the sensor uses a CRC-8-ATM,
   * Polynomial: x8 + x2 + x + 1
   * Corresponds to: 100000111
   */
  #define POLY_8  0x8380
  uint8_t i = 0;
  uint16_t val = 0;

  val = (uint16_t)(a^b);
  val <<= 8;
  for(i=0; i<8; i++) {
    if (val&0x8000 ){
      val ^= POLY_8;
    }
    val <<= 1;
  }
  return (uint8_t)(val >> 8);
}

static uint8_t calcCRC_WriteAccess16(uint8_t i2cSlaveAddr, uint8_t cmd, uint8_t low, uint8_t high) {
  uint8_t crc;

  crc = calcCrc8Atm(0x00, i2cSlaveAddr<<1); /* I2C slave address */
  crc = calcCrc8Atm(crc, cmd);              /* command */
  crc = calcCrc8Atm(crc, low);              /* data byte */
  return calcCrc8Atm(crc, high);            /* data byte */
}

static uint8_t calcCRC_ReadAccess16(uint8_t i2cSlaveAddr, uint8_t cmd, uint8_t low, uint8_t high) {
  uint8_t crc;

  crc = calcCrc8Atm(0x00, i2cSlaveAddr<<1);     /* I2C slave address */
  crc = calcCrc8Atm(crc, cmd);                  /* command */
  crc = calcCrc8Atm(crc, (i2cSlaveAddr<<1)|1);  /* I2C address with R bit set */
  crc = calcCrc8Atm(crc, low);                  /* data byte */
  return calcCrc8Atm(crc, high);                /* data byte */
}

static uint8_t CheckCrc(uint8_t i2cSlaveAddr, uint8_t cmd, uint8_t low, uint8_t high, uint8_t crc) {
  uint8_t val;

  val = calcCRC_ReadAccess16(i2cSlaveAddr, cmd, low, high);
  if (val != crc) {
    return ERR_FAILED;
  }
  return ERR_OK;
}


static void CheckI2CErr(uint8_t res) {
  if (res==ERR_OK) {
    return; /* ok */
  }
  for(;;) { /* I2C Error? */
    LED_R_Neg();
    WAIT1_WaitOSms(50);
  }
}

#include "PORT_PDD.h"
#include "GPIO_PDD.h"
void LCwakeup(void) {
  /* SDA: PTB3
   * SCL: PTB2
   */
  /* Generates wake up signal on SDA, according to the data sheet.
   * This has to happen before any other I2C communication on the bus:
   * Pull down SDA for 0.6us, then high again,
   * wait for 400 us
   *  */
  PORT_PDD_SetPinMuxControl(PORTB_BASE_PTR, 3, PORT_PDD_MUX_CONTROL_ALT1); /* MUX SDA/PTB3 as GPIO */
  GPIO_PDD_SetPortOutputDirectionMask(PTB_BASE_PTR, (1<<3)); /* SDA/PTB3 as output */
  GPIO_PDD_ClearPortDataOutputMask(PTB_BASE_PTR, 1<<3); /* SDA/PB3 low */
  WAIT1_Waitus(1);                                        /* SDA min 0.6us low */
  GPIO_PDD_SetPortDataOutputMask(PTB_BASE_PTR, 1<<3);   /* SDA/PB3 high */
  WAIT1_Waitus(400);                                      /* wait 400us */
  PORT_PDD_SetPinMuxControl(PORTB_BASE_PTR, 3, PORT_PDD_MUX_CONTROL_ALT7); /* MUX SDA/PTB3 as I2C pin */
}

void LCinit(void) {
  /* initializes LC709203F for Renata ICP543759PMT battery */
  uint8_t data_w[3];
  uint8_t result;
  uint8_t crc;

  data_w[0] = 0x01;   //0x0001 = operational mode, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, lC709203F_REG_PW_MODE, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, lC709203F_REG_PW_MODE, data_w, sizeof(data_w));     //set device to operational mode
  CheckI2CErr(result);

  data_w[0] = 0x20;   //0x0020 = 32mOhm, type 1, 3.7V, 4.2V, 1260mAh, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ADJ_APPLI, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ADJ_APPLI, data_w, sizeof(data_w));     //set APA (parasitic impedance)
  CheckI2CErr(result);

  data_w[0] = 0x01;   //0x0001 = Type 1, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_CHG_PARAM, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_CHG_PARAM, data_w, sizeof(data_w));     //set Battery profile
  CheckI2CErr(result);

  data_w[0] = 0x55;   //write 0xAA55 to initialize RSOC, low byte first
  data_w[1] = 0xAA;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_INIT_RSOC, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_INIT_RSOC, data_w, sizeof(data_w));     //initial RSOC
  CheckI2CErr(result);

  data_w[0] = 0x01;   //0x0001 = thermistor mode, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_EN_NTC, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_EN_NTC, data_w, sizeof(data_w));     //set device to thermistor mode
  CheckI2CErr(result);

  data_w[0] = 0x6B;   //0x0D6B = B=3435, low byte first
  data_w[1] = 0x0D;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_SET_THERMB, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_SET_THERMB, data_w, sizeof(data_w));     //set B-constant of Thermistor
  CheckI2CErr(result);

  data_w[0] = 0x1C;   //0x0C1C = 3100mV = 3.1V, low byte first
  data_w[1] = 0x0C;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_LOW_CELL_VOL_ALM, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_LOW_CELL_VOL_ALM, data_w, sizeof(data_w));     //set alarm low cell voltage
  CheckI2CErr(result);

  data_w[0] = 0x00;   //0x0000 = alarm disable, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_RSOC_ALM, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_RSOC_ALM, data_w, sizeof(data_w));     //disable alarm low RSOC
  CheckI2CErr(result);
}

//returns cell voltage in mV
int LCgetVoltage(void) {
  unsigned char data[3];
  int voltage;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_VOLTAGE, data, sizeof(data));
  CheckI2CErr(result);
  CheckI2CErr(CheckCrc(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_VOLTAGE, data[0], data[1], data[2]));
  voltage = (data[1]<<8) | data[0];             //low byte first sent
  return voltage;
}

//returns cell temperature (10 = 1°C)
int LCgetTemp(void) {
  unsigned char data[3];
  int temp;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_CELL_TEMP, data, sizeof(data));
  CheckI2CErr(result);
  temp = ((data[1]<<8) | data[0])-0x0AAC;       //low byte first sent
  CheckI2CErr(CheckCrc(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_CELL_TEMP, data[0], data[1], data[2]));
  return temp;
}

//returns battery charge in percent
int LCgetRSOC(void) {
  unsigned char data[3];
  int RSOC;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_RSOC, data, sizeof(data));
  CheckI2CErr(result);
  CheckI2CErr(CheckCrc(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_RSOC, data[0], data[1], data[2]));
  RSOC = (data[1]<<8) | data[0];                //low byte first sent
  return RSOC;
}

//returns battery charge in thousandth
int LCgetITE(void) {
  unsigned char data[3];
  int ITE;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ITE, data, sizeof(data));
  CheckI2CErr(result);
  CheckI2CErr(CheckCrc(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ITE, data[0], data[1], data[2]));
  ITE = (data[1]<<8) | data[0];                 //low byte first sent
  return ITE;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  int temp, rsoc, ite, mVolt;

  CLS1_SendStatusStr((unsigned char*)"LC", (const unsigned char*)"\r\n", io->stdOut);

  rsoc = LCgetRSOC(); /* battery charge in percent */
  buf[0] = '\0';
  UTIL1_strcatNum16s(buf, sizeof(buf), rsoc);
  UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"\% (0..100)\r\n");
  CLS1_SendStatusStr((unsigned char*)"  RSOC", buf, io->stdOut);

  ite = LCgetITE(); /* battery charge in thousandth */
  buf[0] = '\0';
  UTIL1_strcatNum16s(buf, sizeof(buf), ite);
  UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)" (0..1000)\r\n");
  CLS1_SendStatusStr((unsigned char*)"  ITE", buf, io->stdOut);

  temp = LCgetTemp(); /* cell temperature in 1/10-degree C */
  buf[0] = '\0';
  UTIL1_strcatNum16s(buf, sizeof(buf), temp/10);
  UTIL1_chcat(buf, sizeof(buf), '.');
  if (temp<0) { /* make it positive */
    temp = -temp;
  }
  UTIL1_strcatNum16s(buf, sizeof(buf), temp%10);
  UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"C\r\n");
  CLS1_SendStatusStr((unsigned char*)"  Temperature", buf, io->stdOut);

  mVolt = LCgetVoltage(); /* cell voltage in milli-volts */
  buf[0] = '\0';
  UTIL1_strcatNum16s(buf, sizeof(buf), mVolt);
  UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)" mV\r\n");
  CLS1_SendStatusStr((unsigned char*)"  Voltage", buf, io->stdOut);

  return ERR_OK;
}

uint8_t LC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint8_t res = ERR_OK;
  const uint8_t *p;
  int32_t tmp;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "LC help")==0) {
    CLS1_SendHelpStr((unsigned char*)"LC", (const unsigned char*)"Group of LC709203F commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  } else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "LC status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  }
  return res;
}

void LC_Init(void) {
 // LCwakeup();
  LCinit();
}

#endif /* PL_CONFIG_HAS_GAUGE_SENSOR */
