#pragma once
#include "synthfunctions.h"

typedef long int32;

class RSine {
public:

void initRSine(int32* note, int ratioLevel, float sampleRate);

void renderRSine();

RSine() {
	rSineLev = 0.0;
	mod0 = 0.0;
	mod1 = 0.0;
	rSineModulo = 0.0f;
	rSineDerivative = 0.0f;
	rSineX = 0.0f;
	rSineRatio = 0.0;
	dmod = 0.0;
	ifs = 0.0;
	tester = 4;
	
};
//for recurrance relationhip sine
float rSineLev; //(float)exp(0.05776226505f * ((float)(*note) - 1.0f));
float mod0;
float mod1;
float rSineModulo;
float rSineDerivative;
float rSineX;
float dmod;//2.0f * (float)cos(rSineRatio * rSineLev);
float rSineRatio;//1.570796326795f * ratioPImX;
float tune;
float ifs;
float tester;
};