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

#define LC709203F_REG_CELL_TEMP   0x08  /* Cell temperature */
#define LC709203F_REG_VOLTAGE     0x09  /* Cell voltage */
#define LC709203F_REG_RSOC        0x0D  /* RSOC, value based 0-100 scale */
#define LC709203F_REG_ITE         0x0F  /* ITE, Indicator to Empty */

/*
 * Hinweis:
 * Ich habe die Überprüfung der empfangenen Werte mittels dem CRC8 Wert nicht implementiert.
 * Den CRC8 Wert für die zu sendenden Daten habe ich mittels unten stehender Website berechnet.
 * (das IC akzeptiert die Daten nur mit CRC8 Wert)
 * CRC8 value calculated with: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
 */

static uint8_t i2cReadCmdData(uint8_t i2cAddr, uint8_t cmd, uint8_t *data, size_t length) {
  return GI2C1_ReadAddress(i2cAddr, &cmd, sizeof(cmd), data, length);
}

static uint8_t i2cWriteCmdData(uint8_t i2cAddr, uint8_t cmd, uint8_t *data, size_t length) {
  return GI2C1_WriteAddress(i2cAddr, &cmd, sizeof(cmd), data, length);
}

static uint8_t calcCrc(uint8_t a, uint8_t b) {
  return 0; /* \todo */
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

//Generates wake up signal on SDA \todo
void LCwakeup(void) {
  PORTD_PCR3 = PORT_PCR_MUX(1) | PORT_PCR_ODE(1);   //SDA as gpio
  GPIOD_PDDR |= 0x8;
  GPIOD_PCOR |= 0x8;
  WAIT1_Waitus(1);                                  //SDA min 0.6us low
  GPIOD_PSOR |= 0x8;                                //SDA high
  WAIT1_Waitus(400);                                //wait 400us
  PORTD_PCR3 = PORT_PCR_MUX(7) | PORT_PCR_ODE(1);   //mux SDA
}

//initializes LC709203F for renata ICP543759PMT battery
void LCinit(void) {
  uint8_t data_w[3];
  uint8_t result;

  data_w[0] = 0x01;   //0x0001 = operational mode, low byte first
  data_w[1] = 0x00;
  data_w[2] = 0x64;   //crc8 value of address, cmd & data bytes (0x16 0x15 0x01 0x00)
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x15, data_w, sizeof(data_w));     //set device to operational mode
  CheckI2CErr(result);

  data_w[0] = 0x20;   //0x0020 = 32mOhm, type 1, 3.7V, 4.2V, 1260mAh, low byte first
  data_w[1] = 0x00;
  data_w[2] = 0x51;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x0B, data_w, sizeof(data_w));     //set APA (parasitic impedance)
  CheckI2CErr(result);

  data_w[0] = 0x01;   //0x0001 = Type 1, low byte first
  data_w[1] = 0x00;
  data_w[2] = 0x72;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x12, data_w, sizeof(data_w));     //set Battery profile
  CheckI2CErr(result);

  data_w[0] = 0x55;   //write 0xAA55 to initialize RSOC, low byte first
  data_w[1] = 0xAA;
  data_w[2] = 0x17;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x07, data_w, sizeof(data_w));     //initial RSOC
  CheckI2CErr(result);

  data_w[0] = 0x01;   //0x0001 = thermistor mode, low byte first
  data_w[1] = 0x00;
  data_w[2] = 0xD9;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x16, data_w, sizeof(data_w));     //set device to thermistor mode
  CheckI2CErr(result);

  data_w[0] = 0x6B;   //0x0D6B = B=3435, low byte first
  data_w[1] = 0x0D;
  data_w[2] = 0x2F;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x06, data_w, sizeof(data_w));     //set B-constant of Thermistor
  CheckI2CErr(result);

  data_w[0] = 0x1C;   //0x0C1C = 3100mV = 3.1V, low byte first
  data_w[1] = 0x0C;
  data_w[2] = 0x95;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x14, data_w, sizeof(data_w));     //set alarm low cell voltage
  CheckI2CErr(result);

  data_w[0] = 0x00;   //0x0000 = alarm disable, low byte first
  data_w[1] = 0x00;
  data_w[2] = 0x0C;   //crc8 value of address, cmd & data bytes
  result = i2cWriteCmdData(LC709203F_I2C_SLAVE_ADDR, 0x13, data_w, sizeof(data_w));     //disable alarm low RSOC
  CheckI2CErr(result);
}

//returns cell voltage in mV
int LCgetVoltage(void) {
  unsigned char data[3];
  int voltage;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_VOLTAGE, data, sizeof(data));
  CheckI2CErr(result);
  voltage = (data[1]<<8) | data[0];             //low byte first sent
  //data[2] contains CRC8 value
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
  //data[2] contains CRC8 value
  return temp;
}

//returns battery charge in percent
int LCgetRSOC(void) {
  unsigned char data[3];
  int RSOC;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_RSOC, data, sizeof(data));
  CheckI2CErr(result);
  RSOC = (data[1]<<8) | data[0];                //low byte first sent
  //data[2] contains CRC8 value
  return RSOC;
}

//returns battery charge in thousandth
int LCgetITE(void) {
  unsigned char data[3];
  int ITE;
  uint8_t result;

  result = i2cReadCmdData(LC709203F_I2C_SLAVE_ADDR, LC709203F_REG_ITE, data, sizeof(data));
  CheckI2CErr(result);
  ITE = (data[1]<<8) | data[0];                 //low byte first sent
  //data[2] contains CRC8 value
  return ITE;
}

static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  int temp, rsoc, ite;

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
  CLS1_SendStatusStr((unsigned char*)"  RSOC", buf, io->stdOut);

  temp = LCgetTemp(); /* cell temperature in deci-degree C */
  buf[0] = '\0';
  UTIL1_strcatNum16s(buf, sizeof(buf), temp/10);
  UTIL1_chcat(buf, sizeof(buf), '.');
  if (temp<0) { /* make it positive */
    temp = -temp;
  }
  UTIL1_strcatNum16s(buf, sizeof(buf), temp%10);
  UTIL1_strcat(buf, sizeof(buf), (const unsigned char*)"C\r\n");
  CLS1_SendStatusStr((unsigned char*)"  Temp", buf, io->stdOut);
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
  //LCwakeup(); /* \todo */
  LCinit();
}

#if 0
void main(void)
{
  int voltage=0, temp = 0, RSOC=0, ITE=0;
  i2cInit();
  LCwakeup();
  LCinit();

  while(TRUE)
  {
    voltage = LCgetVoltage();
    temp = LCgetTemp();
    RSOC = LCgetRSOC();
    ITE = LCgetITE();

  }
}
#endif

#endif /* PL_CONFIG_HAS_GAUGE_SENSOR */
