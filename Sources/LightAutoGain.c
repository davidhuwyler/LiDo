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
static int32_t LiGain_GetBiggestDeviation(liDoSample_t* sample)
{
	int32_t deviation;
	int32_t tempDeviation;
	bool isNegative = FALSE;
	bool tempIsNegative = FALSE;

	deviation = LiGain_abs(sample->lightChannelX-LIGAIN_SENSVALUE_TARGETVALUE,&isNegative);

	tempDeviation = LiGain_abs(sample->lightChannelY-LIGAIN_SENSVALUE_TARGETVALUE,&tempIsNegative);
	if(tempDeviation>deviation)
	{
		deviation = tempDeviation;
		isNegative = tempIsNegative;
	}

	tempDeviation = LiGain_abs(sample->lightChannelZ-LIGAIN_SENSVALUE_TARGETVALUE,&tempIsNegative);
	if(tempDeviation>deviation)
	{
		deviation = tempDeviation;
		isNegative = tempIsNegative;
	}

	tempDeviation = LiGain_abs(sample->lightChannelIR-LIGAIN_SENSVALUE_TARGETVALUE,&tempIsNegative);
	if(tempDeviation>deviation)
	{
		deviation = tempDeviation;
		isNegative = tempIsNegative;
	}

	tempDeviation = LiGain_abs(sample->lightChannelB440-LIGAIN_SENSVALUE_TARGETVALUE,&tempIsNegative);
	if(tempDeviation>deviation)
	{
		deviation = tempDeviation;
		isNegative = tempIsNegative;
	}

	tempDeviation = LiGain_abs(sample->lightChannelB490-LIGAIN_SENSVALUE_TARGETVALUE,&tempIsNegative);
	if(tempDeviation>deviation)
	{
		deviation = tempDeviation;
		isNegative = tempIsNegative;
	}

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
	deviation = LiGain_GetBiggestDeviation(lastSample);
	if(deviation <= LIGAIN_SENSVALUE_HYSTERESYS) //Lightsensorvalues in hysteresys --> No adjustment
	{
		*newIntTimeParam = lastSample->lightIntTime;
		*newGainParam = lastSample->lightGain;
	}
	else //Lightsensorvalues out of  hysteresys --> adjustment needed!
	{
		uint32_t oldGainFactor,newGainFactor;
		LiGain_GetGainFactor(lastSample->lightIntTime, lastSample->lightGain,&oldGainFactor);
		newGainFactor = (oldGainFactor * LIGAIN_SENSVALUE_TARGETVALUE) / ( LIGAIN_SENSVALUE_TARGETVALUE + deviation);
		LiGain_GetLightSensParams(newIntTimeParam, newGainParam,newGainFactor);
	}
	return ERR_OK;
}
