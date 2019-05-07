/*
 * LightAutoGain.c
 *
 *  Created on: 01.05.2019
 *      Author: dhuwiler
 */
#include "LightAutoGain.h"

#define LIGAIN_1___RANGE_MAX 35
#define LIGAIN_36__RANGE_MAX 159
#define LIGAIN_160_RANGE_MAX 639
#define LIGAIN_640_RANGE_MAX 163840

#define LIGAIN_SENSVALUE_TARGETVALUE 0x7FFF // = 0xFFFF/2
#define LIGAIN_SENSVALUE_HYSTERESYS  0x3FFF // = 0xFFFF/4

#define LIGAIN_MIN_ALLOWED_INTTIME_PARAM 180

static int32_t LiGain_abs (int32_t i,bool* isNegative)
{
	if(i<0)
	{
		*isNegative = TRUE;
		return -i;
	}
	else
	{
		*isNegative = FALSE;
		return i;
	}
}

static uint8_t LiGain_GetGainFactor (uint8_t intTimeParam, uint8_t gainParam,uint32_t* gainFactor)
{
	uint32_t internalGain;
	uint32_t internalIntT;

	if(gainParam > 3)
	{
		return ERR_FAILED;
	}

	switch(gainParam)
	{
		case 0:
			internalGain = 10;
		break;
		case 1:
			internalGain = 36;
		break;
		case 2:
			internalGain = 160;
		break;
		case 3:
			internalGain = 640;
		break;
	}

	internalIntT = 256 - intTimeParam;
	*gainFactor = internalIntT*internalGain;
	return ERR_OK;
}

static uint8_t LiGain_GetLightSensParams(uint8_t* intTimeParam, uint8_t* gainParam,uint32_t gainFactor)
{
	uint32_t internalGain;
	uint32_t internalIntT;

	if(gainFactor<=LIGAIN_1___RANGE_MAX)
	{
		internalGain = 10;
		*gainParam = 0;
	}
	else if(gainFactor<=LIGAIN_36__RANGE_MAX)
	{
		internalGain = 36;
		*gainParam = 1;
	}
	else if(gainFactor<=LIGAIN_160_RANGE_MAX)
	{
		internalGain = 160;
		*gainParam = 2;
	}
	else if(gainFactor<=LIGAIN_640_RANGE_MAX)
	{
		internalGain = 640;
		*gainParam = 3;
	}
	else
	{
		gainFactor = LIGAIN_640_RANGE_MAX;
		internalGain = 640;
		*gainParam = 3;
	}

	internalIntT = gainFactor / internalGain;
	*intTimeParam = 256 - internalIntT;
	return ERR_OK;
}

//Returns the biggest deviation from the targetvalue from the Lightsensorvalues
static int32_t LiGain_GetYdeviation(liDoSample_t* sample)
{
	int32_t deviation;
	bool isNegative = FALSE;
	deviation = LiGain_abs(sample->lightChannelY-LIGAIN_SENSVALUE_TARGETVALUE,&isNegative);

	if(isNegative)
	{
		return -deviation;
	}
	else
	{
		return deviation;
	}
}

uint8_t LiGain_Compute(liDoSample_t* lastSample,uint8_t* newIntTimeParam, uint8_t* newGainParam)
{
	int32_t deviation;
	bool temp;
	uint8_t err = ERR_OK;
	deviation = LiGain_GetYdeviation(lastSample);

	//Y Value is in hysteresys --> No adjustment:
	if(LiGain_abs(deviation,&temp) <= LIGAIN_SENSVALUE_HYSTERESYS)
	{
		*newIntTimeParam = lastSample->lightIntTime;
		*newGainParam = lastSample->lightGain;
	}

	//Lightsensorvalues out of  hysteresys --> adjustment needed!
	else
	{
		uint32_t oldGainFactor,newGainFactor;
		err = LiGain_GetGainFactor(lastSample->lightIntTime, lastSample->lightGain,&oldGainFactor);
		newGainFactor = (oldGainFactor * LIGAIN_SENSVALUE_TARGETVALUE) / ( LIGAIN_SENSVALUE_TARGETVALUE + deviation);
		err = LiGain_GetLightSensParams(newIntTimeParam, newGainParam,newGainFactor);

		if(*newIntTimeParam<LIGAIN_MIN_ALLOWED_INTTIME_PARAM)
		{
			*newIntTimeParam = LIGAIN_MIN_ALLOWED_INTTIME_PARAM;
		}
	}
	return err;
}
