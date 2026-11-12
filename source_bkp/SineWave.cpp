#include "SineWave.h"

double SineWave::parabolicSine(double x, bool bHighPrecision)
{
	SineWave::pbSinY = SineWave::swB * x + SineWave::swC * x * fabs(x);
	//if(bHighPrecision)
	//SineWave::pbSinY = SineWave::swP * (SineWave::pbSinY * fabs(SineWave::pbSinY) - SineWave::pbSinY) + SineWave::pbSinY;
	return SineWave::pbSinY;
}


double SineWave::BhaskaraISine(double x)
{
	SineWave::bkSinD = fabs(x);
	SineWave::bkSgn = SineWave::d == 0 ? 1.0 : x / fabs(x);
	return 16.0*x*((float)pi - SineWave::bkSgn * x) / (-SineWave::bkSgn * 4.0*x*((float)pi - SineWave::bkSgn * x));
}

double SineWave::swB = 4.0 / (float)pi;
double SineWave::swC = -4.0 / ((float)pi*(float)pi);
double SineWave::swP = 0.225;
double SineWave::pbSinY;
double SineWave::swD = 5.0*(float)pi*(float)pi;
double SineWave::bkSinD;
double SineWave::bkSgn;
double SineWave::d;

