#pragma once
#include "synthfunctions.h"


class SineWave {
public:

	static double SineWave::swB;
	static double SineWave::swC;
	static double SineWave::swP;
	static double SineWave::pbSinY;
	static double SineWave::swD;
	static double SineWave::bkSinD;
	static double SineWave::bkSgn;
	static double SineWave::d;

	static double SineWave::parabolicSine(double x, bool bHighPrecision = true);

	static double SineWave::BhaskaraISine(double x);

	

};

