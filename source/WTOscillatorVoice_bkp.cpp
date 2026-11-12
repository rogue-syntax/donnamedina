#include "WTOscillatorVoice.h"

CWTOscillatorVoice::CWTOscillatorVoice(void)
{
	// clear out arrays
	//no do this in wttables
	//memset(wTTables->m_pSawTables, 0, NUM_TABLES*sizeof(double*));
	//memset(wTTables->m_pTriangleTables, 0, NUM_TABLES*sizeof(double*));

	// init variables
	m_dReadIndex = 0.0;
	m_dWT_inc = 0.0;
	m_dReadIndex2 = 0.0;
	m_dWT_inc2 = 0.0;
	m_nCurrentTableIndex = 0;

	// setup correction factors (empirical)
	m_dSquareCorrFactor[0] = 0.5;
	m_dSquareCorrFactor[1] = 0.5;
	m_dSquareCorrFactor[2] = 0.5;
	m_dSquareCorrFactor[3] = 0.49;
	m_dSquareCorrFactor[4] = 0.48;
	m_dSquareCorrFactor[5] = 0.468;
	m_dSquareCorrFactor[6] = 0.43;
	m_dSquareCorrFactor[7] = 0.34;
	m_dSquareCorrFactor[8] = 0.25;

	// default to SINE
	

	
	tableChoice[0] = &CWTOscillatorVoice::selectNone;
	tableChoice[1] = &CWTOscillatorVoice::selectSaw;
	tableChoice[2] = &CWTOscillatorVoice::selectSaw;
	tableChoice[3] = &CWTOscillatorVoice::selectSaw;
	tableChoice[4] = &CWTOscillatorVoice::selectTri;
	tableChoice[5] = &CWTOscillatorVoice::selectSaw;

	

}


void CWTOscillatorVoice::setSampleRate(double* dFs)
{
	// call base class first
	COscillator::setSampleRate(dFs);
	//m_dSampleRate = dFs;

}

CWTOscillatorVoice::~CWTOscillatorVoice(void)
{
	
}

void CWTOscillatorVoice::reset()
{
	COscillator::reset();

	// back to top of buffer
	m_dReadIndex = 0.0;
	m_dReadIndex2 = 0.0;

	update();
}

void CWTOscillatorVoice::startOscillator()
{
	reset();
	m_bNoteOn = true;
}

void CWTOscillatorVoice::stopOscillator()
{
	m_bNoteOn = false;
}

void CWTOscillatorVoice::update()
{
	COscillator::update();

	m_dWT_inc = WT_LENGTH*m_dInc;

	// select the table
	selectTable();
}



// get table index based on current m_dFo
int CWTOscillatorVoice::getTableIndex()
{
	if(m_uWaveform == SINE)
		return -1;

	double dSeedFreq = 27.5; // Note A0, bottom of piano
	for(int j = 0; j < NUM_TABLES; j++)
	{
		if(m_dFo <= dSeedFreq)
		{
			return j;
		}

		dSeedFreq *= 2.0;
	}

	return -1;
}

void CWTOscillatorVoice::selectTable()
{
	m_nCurrentTableIndex = getTableIndex();

	// if the frequency is high enough, the sine table will be returned
	// even for non-sinusoidal waves; anything about 10548 Hz is one
	// harmonic only (sine)
	if(m_nCurrentTableIndex < 0)
	{
		m_pCurrentTable = &wTTables->m_dSineTable[0];
		return;
	}


	//(this->*tableChoice[m_uWaveform])();
	

	// choose table
	
	if(m_uWaveform == SAW1 || m_uWaveform == SAW2 || m_uWaveform == SAW3 || m_uWaveform == SQUARE)
		m_pCurrentTable = wTTables->m_pSawTables[m_nCurrentTableIndex];
	else if(m_uWaveform == TRI)
		m_pCurrentTable = wTTables->m_pTriangleTables[m_nCurrentTableIndex];
	

}


int CWTOscillatorVoice::selectSaw() {
	m_pCurrentTable = wTTables->m_pSawTables[m_nCurrentTableIndex];
	return 0;
}
int CWTOscillatorVoice::selectTri() {
	m_pCurrentTable = wTTables->m_pTriangleTables[m_nCurrentTableIndex];
	return 0;
}
int CWTOscillatorVoice::selectNone() {
	return 0;
}


double CWTOscillatorVoice::doWaveTable(double& dReadIndex, double dWT_inc)
{
	double dOut = 0;

	// apply phase modulation, if any
	double dModReadIndex = dReadIndex + m_dPhaseMod*WT_LENGTH;

	// check for multi-wrapping on new read index
	checkWrapIndex(dModReadIndex);

	// get INT part
	int nReadIndex = abs((int)dModReadIndex);

	// get FRAC part
	float fFrac = dModReadIndex - nReadIndex;

	// setup second index for interpolation; wrap the buffer if needed
	int nReadIndexNext = nReadIndex + 1 > WT_LENGTH-1 ? 0 :  nReadIndex + 1;

	// interpolate the output
	dOut = dLinTerp(0, 1, m_pCurrentTable[nReadIndex], m_pCurrentTable[nReadIndexNext], fFrac);
	/*
	//WITHOUT PHASE MOD
	dModReadIndex = dReadIndex * WT_LENGTH;

	// check for multi-wrapping on new read index //WITHOUT PHASE MOD
	checkWrapIndex(dModReadIndex);

	// get INT part //WITHOUT PHASE MOD
	nReadIndex = abs((int)dModReadIndex);

	// get FRAC part //WITHOUT PHASE MOD
	fFrac = dModReadIndex - nReadIndex;

	// setup second index for interpolation; wrap the buffer if needed //WITHOUT PHASE MOD
	nReadIndexNext = nReadIndex + 1 > WT_LENGTH - 1 ? 0 : nReadIndex + 1;

	dOut = dLinTerp(0, 1, m_pCurrentTable[nReadIndex], m_pCurrentTable[nReadIndexNext], fFrac);

	dOut1 = dOut1 - dOut;
	*/
	// add the increment for next time
	dReadIndex += dWT_inc;

	// check for wrap
	checkWrapIndex(dReadIndex);

	return dOut;
}

double CWTOscillatorVoice::doSquareWaveCore(double& dReadIndex, double dWT_inc)
{
	double dPW = m_dPulseWidth/100.0;
	double dPWIndex = dReadIndex + dPW*WT_LENGTH;

	// render first sawtooth using dReadIndex
	double dSaw1 = doWaveTable(dReadIndex, dWT_inc);

	// find the phase shifted output
	// get INT part
	if(dWT_inc >= 0)
	{
		if(dPWIndex >= WT_LENGTH)
			dPWIndex = dPWIndex - WT_LENGTH;
	}
	else
	{
		if(dPWIndex < 0)
			dPWIndex = WT_LENGTH + dPWIndex;
	}

	// render second sawtooth using dPWIndex (shifted)
	double dSaw2 = doWaveTable(dPWIndex, dWT_inc);

	// subtract and correct
	double dSqAmp = m_dSquareCorrFactor[m_nCurrentTableIndex];
	double dOut = dSqAmp*dSaw1 -  dSqAmp*dSaw2;

	// the double sawtooth method creates
	// an output that swings worst case from -2.0 to 0
	// or 0 to +2.0 which is out of bounds for us
	// need to correct for this
	//
	// 25% -> 1.33
	// 50% -> 2.0
	// 75% -> 1.33
	double dCorr = 1.0/dPW;
	if(dPW < 0.5)
		dCorr = 1.0/(1.0-dPW);

	// apply correction
	dOut *= dCorr;

	return dOut;
}

double CWTOscillatorVoice::doSquareWave()
{
	// normal output
	double dSquare = doSquareWaveCore(m_dReadIndex, m_dWT_inc);

	return dSquare;
}

double CWTOscillatorVoice::doOscillate()
{
	if(!m_bNoteOn)
	{
		if(pAuxOutput)
			*pAuxOutput = 0.0;

		return 0.0;
	}

	// if square, it has its own routine
	if(m_uWaveform == SQUARE && m_nCurrentTableIndex >= 0)
	{
		double dOut = doSquareWave();
		if(pAuxOutput)
			*pAuxOutput = dOut;

		return dOut;
	}

	// --- get first output
	double dOutSample = doWaveTable(m_dReadIndex, m_dWT_inc);

 	// --- write outputs
	if(m_pModulationMatrix)
	{
		// write our outputs into their destinations
		m_pModulationMatrix->m_dSources[m_uModDestOutput1] = dOutSample*m_dAmplitude*m_dAmpMod;

		// add quad phase/stereo output (QBL is mono)
		m_pModulationMatrix->m_dSources[m_uModDestOutput2] = dOutSample*m_dAmplitude*m_dAmpMod;
	}

	// mono oscillator
	if(pAuxOutput)
		*pAuxOutput = dOutSample*m_dAmplitude*m_dAmpMod;

	return dOutSample*m_dAmplitude*m_dAmpMod;
}