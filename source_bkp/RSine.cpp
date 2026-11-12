#pragma once
#include "RSine.h"


void RSine::initRSine(int32* note, int ratioLevel, float sampleRate) {
	ifs = 1.0f / sampleRate;//
	rSineRatio = 1.570796326795f * ratioLevel;
	tester = tester;
	tune = (8.175798915644 * ifs * tester);
	rSineLev = exp(0.05776226505f * ((*note)));
	rSineLev = rSineLev * tune;
	mod0 = 0.0;
	mod1 = sin(rSineRatio * rSineLev);
	rSineModulo = 0.0f;
	rSineX = 0.0f;
	dmod = 2.0f * cos(rSineRatio * rSineLev);

	
}

void RSine::renderRSine() {
	dmod = 2.0f * (float)cos(rSineRatio * rSineLev);
	//rSineDerivative = mod0 - mod1;
	rSineModulo = dmod * mod0 - mod1;
	mod1 = mod0;
	mod0 = rSineModulo;
	while (rSineModulo > 1.0f) {
		rSineModulo -= 2.0f;
	}
	while (rSineModulo < -1.0f) {
		rSineModulo += 2.0f;
	}
}