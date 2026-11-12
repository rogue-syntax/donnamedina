#include "LFO.h"


CLFO::CLFO(void)
{
	/*
	doOsc[SINE] = &CLFO::doSine;
	doOsc[SAW1] = &CLFO::doSaw;
	doOsc[TRI] = &CLFO::doTri;
	doOsc[SQUARE] = &CLFO::doSqu;
	m_uLFOMode = sync;
	*/
}

CLFO::~CLFO(void)
{
}

void CLFO::reset()
{
	// call base class
	CLFOscillator::reset();

	// call our update
	update();
}



void CLFO::startOscillator()
{
	// if one shot or sync'd LFO, reset 
	if(m_uLFOMode == sync || m_uLFOMode == shot)
		reset();

	// set flag
	m_bNoteOn = true;
}

void CLFO::stopOscillator()
{
	// clear flag
	m_bNoteOn = false;
}


/*
float CLFO::doOscillate() {

	dQPOut = 0.0;

	checkWrapModulo();

	(this->*doOsc[m_uWaveform])();

	m_dModulo += m_dInc;

	return dOut * m_dAmplitude*m_dAmpMod;
}
*/

