#pragma once
#include <math.h>

class CParamSmooth
{
public:

	void initParamSmoother(float smoothingTimeInMs, float samplingRate, float initValue);

	float smoothParameter(float in);

public:
	float samplingRate;
	float c_twoPi = 6.283185307179586476925286766559f;
	float smoothingTimeInMs;
	float a;
	float b;
	float z;
};
