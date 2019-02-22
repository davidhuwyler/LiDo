/*
 * RTC.c
 *
 *  Created on: Feb 22, 2019
 *      Author: dave
 */
#include "RTC1.h"
#include "PE_Const.h"
#include "CLS1.h"
#include "TmDt1.h"

static LDD_TUserData* rtcUserDataHandle;
static LDD_TDeviceData* rtcDeviceDataHandle;

static LDD_RTC_TTime timeToSet;

uint8_t RTC_setTime(LDD_RTC_TTime* rtcTime)
{
	return RTC1_SetTime(rtcDeviceDataHandle,rtcTime);
}

void RTC_getTime(LDD_RTC_TTime* rtcTime)
{
	RTC1_GetTime(rtcDeviceDataHandle,rtcTime);
}

void RTC_getTimeUnixFormat(uint32_t* rtcTimeUnix)
{
	LDD_RTC_TTime timeFromRTC;
	RTC_getTime(&timeFromRTC);

	TIMEREC time;
	time.Hour = timeFromRTC.Hour;
	time.Min = timeFromRTC.Minute;
	time.Sec = timeFromRTC.Second;

	DATEREC date;
	date.Day = timeFromRTC.Day;
	date.Month = timeFromRTC.Month;
	date.Year = timeFromRTC.Year;

	*rtcTimeUnix = TmDt1_TimeDateToUnixSeconds(&time, &date, 0);
}

void RTC_init(bool softInit)
{
	rtcDeviceDataHandle = RTC1_Init(rtcUserDataHandle,softInit);

	//Init timeToSet with current values values
	RTC_getTime(&timeToSet);

	if(timeToSet.Year>=2099) //RTC1 component needs the year to be smaller than 2099
	{
		timeToSet.Year = 2019;
	}
}

static void PrintTimeDate(CLS1_ConstStdIOType *io)
{
	LDD_RTC_TTime time;
	RTC_getTime(&time);

	CLS1_SendStr("Time: ",io->stdOut);
	CLS1_SendNum32u(time.Hour,io->stdOut);
	CLS1_SendStr(":",io->stdOut);
	CLS1_SendNum32u(time.Minute,io->stdOut);
	CLS1_SendStr(":",io->stdOut);
	CLS1_SendNum32u(time.Second,io->stdOut);

	CLS1_SendStr("  Date: ",io->stdOut);
	CLS1_SendNum32u(time.Day,io->stdOut);
	CLS1_SendStr(".",io->stdOut);
	CLS1_SendNum32u(time.Month,io->stdOut);
	CLS1_SendStr(".",io->stdOut);
	CLS1_SendNum32u(time.Year,io->stdOut);
	CLS1_SendStr("\r\n",io->stdOut);
}


static uint8_t PrintStatus(CLS1_ConstStdIOType *io) {
  uint8_t buf[32];
  CLS1_SendStatusStr((unsigned char*)"RTC", (const unsigned char*)"\r\n", io->stdOut);
  return ERR_OK;
}

uint8_t RTC_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io)
{
	  uint8_t res = ERR_OK;
	  const uint8_t *p;
	  int32_t tmp;

	  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "LightSens help")==0)
	  {
	    CLS1_SendHelpStr((unsigned char*)"RTC", (const unsigned char*)"Group of RTC (RealTimeClock) commands\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  setSeconds", (const unsigned char*)"Sets the current Seconds (0..60)\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  setMinutes", (const unsigned char*)"Sets the current Minutes (0..60)\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  setHours", (const unsigned char*)"Sets the current Hours (0..24)\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  setDay", (const unsigned char*)"Sets the day of the Month (0..31)\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  setMonth", (const unsigned char*)"Sets the Month (1 = Jan.. 12 = Dez.\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  setYear", (const unsigned char*)"Sets the Year (ex. 2019)\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  write", (const unsigned char*)"Saves the set time into the RTC\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  get", (const unsigned char*)"Displays the current time & date\r\n", io->stdOut);
	    CLS1_SendHelpStr((unsigned char*)"  getUnix", (const unsigned char*)"Displays the seconds since Jan 01 1970\r\n", io->stdOut);
	    *handled = TRUE;
	    return ERR_OK;
	  }
	  else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "RTC status")==0))
	  {
	    *handled = TRUE;
	    return PrintStatus(io);
	  }
	  else if (UTIL1_strncmp((char*)cmd, "RTC setSeconds", sizeof("RTC setSeconds")-1)==0)
	  {
		  p = cmd+sizeof("RTC setSeconds")-1;
		  res = UTIL1_xatoi(&p, &tmp);
		  if (res==ERR_OK && tmp>=0 && tmp<=60)
		  {
			  timeToSet.Second = tmp;
		      *handled = TRUE;
		  }
	  }
	  else if (UTIL1_strncmp((char*)cmd, "RTC setMinutes", sizeof("RTC setMinutes")-1)==0)
	  {
		  p = cmd+sizeof("RTC setMinutes")-1;
		  res = UTIL1_xatoi(&p, &tmp);
		  if (res==ERR_OK && tmp>=0 && tmp<=60)
		  {
			  timeToSet.Minute = tmp;
		      *handled = TRUE;
		  }
	  }
	  else if (UTIL1_strncmp((char*)cmd, "RTC setHours", sizeof("RTC setHours")-1)==0)
	  {
		  p = cmd+sizeof("RTC setHours")-1;
		  res = UTIL1_xatoi(&p, &tmp);
		  if (res==ERR_OK && tmp>=0 && tmp<=24)
		  {
			  timeToSet.Hour = tmp;
		      *handled = TRUE;
		  }
	  }
	  else if (UTIL1_strncmp((char*)cmd, "RTC setDay", sizeof("RTC setDay")-1)==0)
	  {
		  p = cmd+sizeof("RTC setDay")-1;
		  res = UTIL1_xatoi(&p, &tmp);
		  if (res==ERR_OK && tmp>=1 && tmp<=31)
		  {
			  timeToSet.Day = tmp;
		      *handled = TRUE;
		  }
	  }
	  else if (UTIL1_strncmp((char*)cmd, "RTC setMonth", sizeof("RTC setMonth")-1)==0)
	  {
		  p = cmd+sizeof("RTC setMonth")-1;
		  res = UTIL1_xatoi(&p, &tmp);
		  if (res==ERR_OK && tmp>=1 && tmp<=12)
		  {
			  timeToSet.Month = tmp;
		      *handled = TRUE;
		  }
	  }
	  else if (UTIL1_strncmp((char*)cmd, "RTC setYear", sizeof("RTC setYear")-1)==0)
	  {
		  p = cmd+sizeof("RTC setYear")-1;
		  res = UTIL1_xatoi(&p, &tmp);
		  if (res==ERR_OK && tmp>=2019 && tmp<=3000)
		  {
			  timeToSet.Year = tmp;
		      *handled = TRUE;
		  }
	  }
	  else if (UTIL1_strcmp((char*)cmd, "RTC write")==0) {
		*handled = TRUE;
		return RTC_setTime(&timeToSet);
	  }
	  else if (UTIL1_strcmp((char*)cmd, "RTC get")==0) {
		PrintTimeDate(io);
		*handled = TRUE;
		return ERR_OK;
	  }
	  else if (UTIL1_strcmp((char*)cmd, "RTC getUnix")==0) {
		uint32_t unixTime;
		RTC_getTimeUnixFormat(&unixTime);
		CLS1_SendNum32u(unixTime,io->stdOut);
		CLS1_SendStr("\r\n",io->stdOut);
		*handled = TRUE;
		return ERR_OK;
	  }
	  return res;
}
