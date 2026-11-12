#include "CParamSmooth.h"




void CParamSmooth::initParamSmoother(float smoothingTimeInMsIn, float samplingRateIn, float initValue)
	{
		a = 0.0; 
		b = 0.0; 
		z = 0.0; 
		smoothingTimeInMs = smoothingTimeInMsIn;
		samplingRate = samplingRateIn;
		a = static_cast<float>(exp(-c_twoPi / (smoothingTimeInMs * 0.001f * samplingRate)));
		b = 1.0f - a;
		z = initValue;
	}

	float CParamSmooth::smoothParameter(float in)
	{
		z = (in * b) + (z * a);
		return z;
	}
