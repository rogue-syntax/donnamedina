#pragma once
#include "voice.h"
#include "WTOscillator.h"
#include "QBLimitedOscillator.h"

#define IMAX 4.0

class CRSSynth002Voice : public CVoice
{
public:
	CRSSynth002Voice(void);
	~CRSSynth002Voice(void);

protected:
	// specific to this voice
	//
	// You can choose the oscillator you like here
	//CWTOscillator m_Op1;
	//CWTOscillator m_Op2;
	//CWTOscillator m_Op3;
	//CWTOscillator m_Op4;

	CQBLimitedOscillator m_Op1;
	
	double m_dOp1Feedback;
	
	// enum for LFO mod
	enum{None,AmpMod,Vibrato};

public:


	
	inline virtual void initGlobalParameters(globalSynthParams* pGlobalParams)
	{
		// --- always call base class first
		CVoice::initGlobalParameters(pGlobalParams);

	}

	// --- Overrides
	//virtual void initializeModMatrix(CModulationMatrix* pMatrix);
	virtual void prepareForPlay();
	virtual void setSampleRate(double dSampleRate);
	virtual void update();
	virtual void reset();

	/// NEEED TO OVERRIDE NOTE OFF!!!! EGS!!!!!!!! WP 3.13.14

	// we are in a state to accept a noteOff message
	// for FM synth, this depends on algorithm, all
	// carrier oscillators must be in A, D or S state
	

	
};
