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
#define LC709203F_REG_SET_THERMB        0x06  /* Thermistor B */
#define LC709203F_REG_INIT_RSOC         0x07  /* Initial RSOC */
#define LC709203F_REG_CELL_TEMP         0x08  /* Cell temperature */
#define LC709203F_REG_VOLTAGE           0x09  /* Cell voltage */
#define LC709203F_REG_CURRENT_DIRECTION 0x0A  /* Current direction */
#define LC709203F_REG_ADJ_APPLI         0x0B  /* APA, Adjustment Pack Application, sets Parasitic impedance */
#define LC709203F_REG_APT               0x0C  /* APT, Adjustment Pack Thermistor, adjust temperature measurement delay timing */
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
void LC_Wakeup(void) {
  /* SDA: PTB3
   * SCL: PTB2
   */
  /* Generates wake up signal on SDA, according to the data sheet.
   * This has to happen before any other I2C communication on the bus:
   * Pull down SDA for 0.6us, then high again,
   * wait for 400 us
   *  */
  PORT_PDD_SetPinMuxControl(PORTB_BASE_PTR, 3, PORT_PDD_MUX_CONTROL_ALT1); /* MUX SDA/PTB3 as GPIO */
  PORT_PDD_SetPinOpenDrain(PORTB_BASE_PTR, 3, PORT_PDD_OPEN_DRAIN_ENABLE);
  GPIO_PDD_SetPortOutputDirectionMask(PTB_BASE_PTR, (1<<3)); /* SDA/PTB3 as output */
  GPIO_PDD_ClearPortDataOutputMask(PTB_BASE_PTR, 1<<3); /* SDA/PB3 low */
  WAIT1_Waitus(1);                                        /* SDA min 0.6us low */
  GPIO_PDD_SetPortDataOutputMask(PTB_BASE_PTR, 1<<3);   /* SDA/PB3 high */
  WAIT1_Waitus(400);                                      /* wait 400us */
  /* mux back to normal I2C mode with interrupts enabled */
  PORTB_PCR3 = (uint32_t)((PORTB_PCR3 & (uint32_t)~(uint32_t)(
                PORT_PCR_ISF_MASK |
                PORT_PCR_MUX(0x05)
               )) | (uint32_t)(
                PORT_PCR_MUX(0x02)
               ));
  PORT_PDD_SetPinOpenDrain(PORTB_BASE_PTR, 0x03u, PORT_PDD_OPEN_DRAIN_ENABLE); /* Set SDA pin as open drain */
}

static void LCinit(void) {
  /* initializes LC709203F for Renata ICP543759PMT battery */
  uint8_t data_w[3];
  uint8_t result;
  uint8_t crc;

  data_w[0] = 0x01;   // Operational mode (1: operational, 2: sleep), 0x0001 = operational mode, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, lC709203F_REG_PW_MODE, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, lC709203F_REG_PW_MODE, data_w, sizeof(data_w));     //set device to operational mode
  CheckI2CErr(result);

  data_w[0] = 0x20;   //APA, set parasitic impedance: 0x0020 = 32mOhm, type 1, 3.7V, 4.2V, 1260mAh, low byte first
  data_w[1] = 0x00;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ADJ_APPLI, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ADJ_APPLI, data_w, sizeof(data_w));     //set APA (parasitic impedance)
  CheckI2CErr(result);

  data_w[0] = 0x01;   //Battery Profile (Table 8 in data sheet): 0x0001 = Type 1, low byte first
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

  data_w[0] = 0x6B;   //Thermistor B-Constant: 0x0D6B = B=3435, low byte first
  data_w[1] = 0x0D;
  data_w[2] = calcCRC_WriteAccess16(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_SET_THERMB, data_w[0], data_w[1]);
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_SET_THERMB, data_w, sizeof(data_w));     //set B-constant of Thermistor
  CheckI2CErr(result);

  data_w[0] = 0x1C;   //Low voltage alarm setting: 0x0C1C = 3100mV = 3.1V, low byte first
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

static uint16_t ReadCmdWordChecked(uint8_t i2cSlaveAddr, uint8_t cmd) {
  uint8_t data[3];

  CheckI2CErr(i2cReadCmdData(i2cSlaveAddr, cmd, data, sizeof(data)));
  CheckI2CErr(CheckCrc(i2cSlaveAddr, cmd, data[0], data[1], data[2]));
  return (data[1]<<8) | data[0];
}

//returns cell voltage in mV
uint16_t LCgetVoltage(void) {
  return ReadCmdWordChecked(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_VOLTAGE);
}

//returns cell temperature (10 = 1°C)
int16_t LCgetTemp(void) {
  /* cell temperature is in 0.1C units, from 0x09E4 (-20C) up to 0x0D04 (60C) */
  int16_t temp;

  temp = ReadCmdWordChecked(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_CELL_TEMP);
  temp -= 0x0AAC; /* From the data sheet command table: 0x0AAC is 0 degree Celsius */
  return (int16_t)temp;
}

//returns battery Relative State of Charge in percent
uint16_t LCgetRSOC(void) {
  return ReadCmdWordChecked(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_RSOC);
}

// Indicator to empty, returns battery charge in thousandth
uint16_t LCgetITE(void) {
  return ReadCmdWordChecked(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ITE);
}

// Indicator to empty, returns battery charge in thousandth
uint16_t LCgetICversion(void) {
  return ReadCmdWordChecked(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_IC_VER);
}

static const unsigned char *LC_CurrentDirectionToString(LC_CurrentDirection dir) {
  switch(dir) {
    case LC_CURRENT_DIR_AUTO:       return (const unsigned char*)"auto";
    case LC_CURRENT_DIR_CHARGING:   return (const unsigned char*)"charging";
    case LC_CURRENT_DIR_DISCHARING: return (const unsigned char*)"discharging";
    case LC_CURRENT_DIR_ERROR:
    default:                        return (const unsigned char*)"ERROR";
  }
}

LC_CurrentDirection LCgetCurrentDirection(void) {
  uint16_t val;

  val = ReadCmdWordChecked(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_CURRENT_DIRECTION);
  switch(val) {
    case 0: return LC_CURRENT_DIR_AUTO;
    case 1: return LC_CURRENT_DIR_CHARGING;
    case 0xffff: return LC_CURRENT_DIR_DISCHARING;
    default: return LC_CURRENT_DIR_ERROR;
  }
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];

  CLS1_SendStatusStr((unsigned char*)"LC", (const unsigned char*)"\r\n", io->stdOut);
  {
    uint16_t version;

    version = LCgetICversion(); /* battery charge in thousandth */
     UTIL1_strcpy(buf, sizeof(buf), "0x");
    UTIL1_strcatNum16Hex(buf, sizeof(buf), version);
    UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"\r\n");
    CLS1_SendStatusStr((unsigned char*)"  IC version", buf, io->stdOut);
  }
  {
    uint16_t rsoc;

    rsoc = LCgetRSOC(); /* battery charge in percent */
    UTIL1_Num16uToStr(buf, sizeof(buf), rsoc);
    UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"\% (0..100)\r\n");
    CLS1_SendStatusStr((unsigned char*)"  RSOC", buf, io->stdOut);
  }
  {
    uint16_t ite;

    ite = LCgetITE(); /* battery charge in thousandth */
    UTIL1_Num16uToStr(buf, sizeof(buf), ite);
    UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)" (0..1000)\r\n");
    CLS1_SendStatusStr((unsigned char*)"  ITE", buf, io->stdOut);
  }
  {
    int16_t temperature;

    temperature = LCgetTemp(); /* cell temperature in 1/10-degree C */
    UTIL1_Num16sToStr(buf, sizeof(buf), temperature/10);
    UTIL1_chcat(buf, sizeof(buf), '.');
    if (temperature<0) { /* make it positive */
      temperature = -temperature;
    }
    UTIL1_strcatNum16s(buf, sizeof(buf), temperature%10);
    UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)" C\r\n");
    CLS1_SendStatusStr((unsigned char*)"  Temperature", buf, io->stdOut);
  }
  {
    uint16_t mVolt;

    mVolt = LCgetVoltage(); /* cell voltage in milli-volts */
    UTIL1_Num16uToStr(buf, sizeof(buf), mVolt);
    UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)" mV\r\n");
    CLS1_SendStatusStr((unsigned char*)"  Voltage", buf, io->stdOut);
  }
  {
    LC_CurrentDirection direction;

    direction = LCgetCurrentDirection();
    UTIL1_strcpy(buf, sizeof(buf), LC_CurrentDirectionToString(direction));
    UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"\r\n");
    CLS1_SendStatusStr((unsigned char*)"  Current dir", buf, io->stdOut);
  }
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
  LCinit();
}

#endif /* PL_CONFIG_HAS_GAUGE_SENSOR */
