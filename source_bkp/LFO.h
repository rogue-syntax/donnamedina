#pragma once
#include "CLFOscillator.h"

class CLFO : public CLFOscillator
{
public:
	CLFO(void);

	~CLFO(void);
	
	// virtual overrides
	virtual void reset();
	virtual void startOscillator();
	virtual void stopOscillator();

	bool chaseModulo = false;
	float chaseModuloStep = 0.0;
	float interimModulo = 0.0;
	float sawSmooth = 25.0;
	float sawModuloInc = 0.0;
	float dOut = 0.0;
	float dQPOut = 0.0;

	inline void doSine() {
		//float dAngle = m_dModulo * 2.0*pi - pi;
		dOut = parabolicSine((m_dModulo * 2.0*pi - pi) * -1.0);
		
	}

	inline void doSaw() {
		//attempt at smoothing out saw lfo
		if (m_dInc > 0 && m_dModulo >= 1.0) {
			chaseModulo = true;
			m_dModulo -= 1.0;
			interimModulo = .9999;
			sawModuloInc = (1000.0*(interimModulo - m_dModulo)) / sawSmooth / (*m_dSampleRate);
			chaseModuloStep = 0;
		}

		if (chaseModulo == true) {
			interimModulo = interimModulo - sawModuloInc;
			dOut = unipolarToBipolar(interimModulo);
			chaseModuloStep = chaseModuloStep + 1;
			if (chaseModuloStep >= sawSmooth) {
				chaseModulo = false;
			}
		}
		else {
			dOut = -1 * unipolarToBipolar(m_dModulo);
		}

	};
	/*
	inline void doSaw() {
		if (m_dInc > 0 && m_dModulo >= 1.0)
		{
			chaseModulo = true;
			m_dModulo -= 1.0;
			interimModulo = .9999;
			sawModuloInc = (1000.0*m_dModulo) / sawSmooth / (*m_dSampleRate);
			chaseModuloStep = 0;
		}
		
		if (chaseModulo == true) {
			interimModulo = m_dModulo - sawModuloInc;
			dOut = unipolarToBipolar(interimModulo);
			chaseModuloStep = chaseModuloStep + 1;
			if (chaseModuloStep >= sawSmooth) {
				chaseModulo = false;
			}
		}
		else {
			dOut = unipolarToBipolar(m_dModulo);
		}
		//for rising edge
		//if last dModulo - currentDModulo > 0 ;means we've reset modulo
		//?
		//or just smooth chase the modulo?
		

		//dOut = unipolarToBipolar(m_dModulo);
	
	}
	*/

	inline void doTri() {

		if (m_dModulo < 0.5) {
			dOut = 2.0*m_dModulo;
		}
		else {
			dOut = 1.0 - 2.0*(m_dModulo - 0.5);
		}

		dOut = unipolarToBipolar(dOut);

	

	}

	inline void doSqu() {
		dOut = m_dModulo > m_dPulseWidth / 100.0 ? -1.0 : +1.0;
	
	}

	int(CLFO::*doOsc[4])();

	//float E1env;
	//float E1cenv;
	//float E1catt;
	//float E1cdec;
	//float E1envInc;



	inline float doOscillate() {
		//update done here


		dQPOut = 0.0;
		switch (m_uWaveform) {
		case 0:
			checkWrapModulo();
			doSine();
			break;
		case 1:
			doSaw();
			break;
		case 2:
			checkWrapModulo();
			doTri();
			break;
		case 3:
			checkWrapModulo();
			doSqu();
			break;
		}


		m_dModulo += m_dInc;
		//cenv = dOut * m_dAmplitude*m_dAmpMod;
		return dOut * m_dAmplitude*m_dAmpMod;

	};


	inline void doEnvOp() {
		e = env;
		env = e * cdec;
		cenv += catt * (e - cenv);
	}

	float e = 0.0;
	float env = 1.0;
	float cenv = 1.0;
	float cdec = 0.0;
	float envInc = 0.0;

	float catt = 0.5;
	float cdecay = 0.5;
	float crel = 0.5;
	float decayReleaseToggle = 0.0;

	float BPM = 0.0;
	int syncBpm = 0;
	float rate = 0.0;
	float level = 0.0;

	int lfoType = 0;

	int toggleGlobal = 0;
	
};
