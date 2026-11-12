#include "QBLimitedOscillator.h"

CQBLimitedOscillator::CQBLimitedOscillator(void)
{

	oscChoice[0] = &CQBLimitedOscillator::doSine;
	oscChoice[1] = &CQBLimitedOscillator::doSawtooth;
	oscChoice[2] = &CQBLimitedOscillator::doTriangle2;
	oscChoice[3] = &CQBLimitedOscillator::doSquare;
	oscChoice[4] = &CQBLimitedOscillator::doSaw2;
	oscChoice[5] = &CQBLimitedOscillator::doSaw3;
	oscChoice[6] = &CQBLimitedOscillator::doNoise;
	oscChoice[7] = &CQBLimitedOscillator::doPNoise;
	oscChoice[8] = &CQBLimitedOscillator::doRSine;

}

CQBLimitedOscillator::~CQBLimitedOscillator(void)
{
}

void CQBLimitedOscillator::reset()
{
	COscillator::reset();
	debugInt = 0;
	/*
	if (isModulator) {
		if (m_uWaveform == TRI) {
			m_dModulo = 0.5;
			//m_dDPWSquareModulator = 0.0;
		}
		
	}
	*/

	// --- saw/tri starts at 0.5

	if (m_uWaveform == SAW1 || m_uWaveform == SAW2 || m_uWaveform == SAW3 ) {
		m_dModulo = 0.5;
	}
	
	

	// --- update if no note playing
	//if(!m_bNoteOn)
		//update();
}

void CQBLimitedOscillator::startOscillator()
{
	reset();
	
	//m_bNoteOn = true;
}

void CQBLimitedOscillator::stopOscillator()
{
	//m_bNoteOn = false;
}

void CQBLimitedOscillator::doSawtooth()
{

	doOscOut = doSawtoothO(dCalcModulo, m_dInc);

	
}



void CQBLimitedOscillator::doRSine() {
	rSine.renderRSine();
	doOscOut = rSine.mod1;
	
}

void CQBLimitedOscillator::doSquare(){
	/*
	double tempModulo;

	tempModulo = dCalcModulo * triCutoff;

	if (dCalcModulo < (triCutoff)) {
		doOscOut = 0;
		fallingEdge = false;
	}
	else {
		doOscOut = 1;
		fallingEdge = true;
	}
	doOscOut = 2 * doOscOut - 1;
	
	postSawBlep();
	*/
	
	// --- sum-of-saws method
	//
	// --- pretend to be SAW1 type
	// --- get first sawtooth output
			// --- Pulse Width Modulation --- //
		// --- limits are 2% and 98%
	//m_dPulseWidth = m_dPulseWidthControl + m_dPWMod * (OSC_PULSEWIDTH_MAX - OSC_PULSEWIDTH_MIN) / OSC_PULSEWIDTH_MIN;
	//m_dPulseWidth = m_dPulseWidth;
	// --- bound the PWM to the range
	
	
	doOscOut = doSquareO(dCalcModulo, m_dInc);
	
	
}

void CQBLimitedOscillator::doTriangle() {


	//doTriOut = 0.0;

	//bDone = false;
	/*
	//crazy effect here
	if (m_dModulo > triCutoff) {
		m_dDPWSquareModulator *= -1.0;
	}
	*/
	




	if (bWrap){
		m_dDPWSquareModulator *= -1.0;
	}
	

	//dBipolar = 2.0*(tanh((triCutoff)*dCalcModulo) / tanh((triCutoff))) - 1.0;
	//double triSample;
	dBipolar = dCalcModulo;
	

	dBipolar = 2.0*(dBipolar) - 1.0;
	//dBipolar = 2 * (2.0*triSample - 1.0) + 1;
	
	dSq = dBipolar * dBipolar;
	
	dInv = 1.0 - dSq;

	// modulation with square modulo
	dSqMod = dInv * m_dDPWSquareModulator;
	//dSqMod = dInv;
	// original
	dDifferentiatedSqMod = dSqMod - m_dDPW_z1;
	m_dDPW_z1 = dSqMod;

	// c = fs/[4fo(1-2fo/fs)]
	c = (*m_dSampleRate) / (4.0*2.0*m_dFo*(1 - m_dInc));

	doOscOut = dDifferentiatedSqMod * c;

	incModulo();
	
	
}

void CQBLimitedOscillator::doTriangle2() {

	//double tempModulo = (tanh(1.5*dCalcModulo) / tanh(1.5));

	double tempModulo = dCalcModulo;
	tempModulo = tempModulo * (triCutoff + 0.5);
	if (tempModulo > 0.5) {//??
		triSample = 1.0 - tempModulo;
		fallingEdge = false;
	}
	else {
		triSample = tempModulo;
		fallingEdge = true;
	}

	
	//c = (*m_dSampleRate) / (4.0*2.0*m_dFo*(1 - m_dInc));

	doOscOut = 2 * (2.0*triSample - 1.0) + 1;
	//dSqMod = 2 * (2.0*triSample - 1.0) + 1;
	//dSqMod = 2.0*triSample - 1.0;
	//dDifferentiatedSqMod = dSqMod - m_dDPW_z1;
	//m_dDPW_z1 = dSqMod;

	// c = fs/[4fo(1-2fo/fs)]
	//c = (*m_dSampleRate) / (4.0*m_dFo*(1 - 2*(m_dInc)));

	//doOscOut = dDifferentiatedSqMod * c;

	//doOscOut = doBlep(dCalcModulo, doOscOut, m_dInc, fallingEdge);
	
}

void CQBLimitedOscillator::doSine() {
	// calculate angle
	dAngle = dCalcModulo * 2.0*(double)pi - (double)pi;
	//dAngle = m_dModulo * 2.0*(double)pi - (double)pi;
	// call the parabolicSine approximator or other
	//doOscOut = SineWave::BhaskaraISine(-1.0*dAngle);
	doOscOut = SineWave::parabolicSine(-1.0*dAngle);

	
	//postSawBlep();
	
}






void CQBLimitedOscillator::doSaw1() {
	/*
	doOscOut = 2.0*dCalcModulo - 1.0;
	dDifferentiatedSqMod = doOscOut - m_dDPW_z1;
	m_dDPW_z1 = doOscOut;
	doOscOut = dDifferentiatedSqMod;
	*/
	
	dBipolar = 2.0*(dCalcModulo)-1.0;


	dSq = dBipolar * dBipolar;

	// inversion
	dInv = 1.0 - dSq;

	// modulation with square modulo
	dSqMod = dInv * m_dDPWSquareModulator;

	// original
	dDifferentiatedSqMod = dSqMod - m_dDPW_z1;
	m_dDPW_z1 = dSqMod;

	// c = fs/[4fo(1-2fo/fs)]
	c = (*m_dSampleRate) / (4.0*2.0*m_dFo*(1 - m_dInc));

	doOscOut = dDifferentiatedSqMod * c;

	incModulo();

	//fallingEdge = false;
	//postSawBlep();
	
}

void CQBLimitedOscillator::doSaw2() {
	doOscOut = 2.0*(tanh(1.5*dCalcModulo) / tanh(1.5)) - 1.0;
	postSawBlep();
	fallingEdge = false;
	
}

void CQBLimitedOscillator::doSaw3() {
	doOscOut = 2.0*dCalcModulo - 1.0;
	doOscOut = tanh(1.5*doOscOut) / tanh(1.5);
	fallingEdge = false;
	postSawBlep();
	
}

void CQBLimitedOscillator::doNoise() {
	doOscOut = doWhiteNoise();
	
}

void CQBLimitedOscillator::doPNoise() {
	doOscOut = doPNSequence(m_uPNRegister);
	
}

void CQBLimitedOscillator::postTriBlep() {
	if (m_dFo <= (*m_dSampleRate) / 40) {
		//if (debugInt == 0) {
		//	OutputDebugStringA("40");
		//	debugInt = 1;
		//}

		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_TRI[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			fallingEdge,
			4,
			true);
	}
	else if (m_dFo <= (*m_dSampleRate) / 10) {

		//if (debugInt == 0) {
		//	OutputDebugStringA("20");
		//	debugInt = 1;
		//}
		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_TRI[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			fallingEdge, /* falling edge */
			2,
			true);
	}
	else {
		//if (debugInt == 0) {
		//	OutputDebugStringA("10");
		//	debugInt = 1;
		//}
		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_TRI[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			fallingEdge,	/* falling edge */
			1,
			true);
	}
}

void CQBLimitedOscillator::postSquareBlep() {
	if (m_dFo <= (*m_dSampleRate) / 40) {
		//if (debugInt == 0) {
		//	OutputDebugStringA("40");
		//	debugInt = 1;
		//}

		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_RECT[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			false,
			4,
			false);
	}
	else if (m_dFo <= (*m_dSampleRate) / 10) {

		//if (debugInt == 0) {
		//	OutputDebugStringA("20");
		//	debugInt = 1;
		//}
		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_RECT[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			false, /* falling edge */
			2,
			false);
	}
	else {
		//if (debugInt == 0) {
		//	OutputDebugStringA("10");
		//	debugInt = 1;
		//}
		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_RECT[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			false,	/* falling edge */
			1,
			true);
	}
}

void CQBLimitedOscillator::postSawBlep() {
	//doOscOut = doPolyBLEP_2(dCalcModulo,		/* current phase value */
	//	fabs(m_dInc),	/* abs(dInc) is for FM synthesis with negative frequencies */
	//	1.0,			/* sawtooth edge height = 1.0 */
	//	false );
	//HOW DO WE GET 40 FROM NOTE THATCORERRESPONDS TO  (*m_dSampleRate) / 40,  10 FROM (*m_dSampleRate) / 10, 1 FROM (*m_dSampleRate) / 1
	//COULD JUST INCLUDE THAT FACTOR VS CONDITIONALS...
	if (m_dFo <= (*m_dSampleRate) / 40) {
		//if (debugInt == 0) {
		//	OutputDebugStringA("40");
		//	debugInt = 1;
		//}
		
		doOscOut = doOscOut + doBLEP_N(&dBLEPTable_8_BLKHAR[0],
			4096,
			dCalcModulo,
			fabs(m_dInc),
			1.0,
			fallingEdge,
			4,
			false);
	}else if (m_dFo <= (*m_dSampleRate) / 10) {
		
		//if (debugInt == 0) {
		//	OutputDebugStringA("20");
		//	debugInt = 1;
		//}
		doOscOut = doOscOut + doBLEP_N(&dBLEPTable[0],
		4096,
		dCalcModulo,
		fabs(m_dInc),
		1.0,
		fallingEdge, /* falling edge */
		2,
		true);
	}else {
		//if (debugInt == 0) {
		//	OutputDebugStringA("10");
		//	debugInt = 1;
		//}
			doOscOut = doOscOut + doBLEP_N(&dBLEPTable[0],
			4096,			
			dCalcModulo,		
			fabs(m_dInc),	
			1.0,			
			fallingEdge,	/* falling edge */
			2,			
			true);		
	}
	
}

double CQBLimitedOscillator::doOscillate(){
		doOscOut = 0.0;

		
		bWrap = checkWrapModulo2();

		dCalcModulo = m_dModulo + m_dPhaseMod;

		checkWrapIndex(dCalcModulo);
		
		(this->*oscChoice[m_uWaveform])();

		incModulo();


		return doOscOut;



}
