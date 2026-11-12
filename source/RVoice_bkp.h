
#include "SynthFunctions.h"
#include "WTOscillatorVoice.h"
#include "constants.h"
#include "QBLimitedOscillator.h"
#include "SynthParamLimits.h"
#include "LFO.h"
#include "MoogLadderFilter.h"
#include "SEMFilter.h"
#include "logscale.h"
#include "EnvelopeGenerator.h"

#include "CParamSmooth.h"

#ifndef __SMTG_OS_WINDOWS__
#define __SMTG_OS_WINDOWS__
#if SMTG_OS_WINDOWS && !defined(__GNUC__)

typedef long int32;
typedef unsigned long uint32;
#else
typedef int int32;
typedef unsigned int uint32;
#endif

#endif

#pragma once
#include <string>
#include <array>



class RVoice {
public:
	//RData() {};
	int id;
	//std::string name;
	int voiceCount;

	RVoice(int idIn, std::string nameIn) : voiceId(idIn) {
		mainCEnvPtr = &E1cenv;

		
	};


	RVoice() : id(0) {
		voiceCount = PARTVOICES;

	};
	//CParamSmooth oscSmooth;
	//CMoogLadderFilter m_Filter1;
	CSEMFilter m_Filter1;
	//CVAOnePoleFilter m_Filter1;
//#define wtOsc 
	//CWTOscillatorVoice WTOsc;
	CQBLimitedOscillator WTOsc2;
	CQBLimitedOscillator WTOsc;

	//CLFO m_LFO1;
	//CLFO m_LFO2;
	//CLFO m_LFO3;
	//float lfo1Out = 1.0;
	//float lfo2Out = 1.0;
	//float lfo3Out = 1.0;
	//Env env1;
	//Env env2;
	//Env env3;
	float envOffOut = 0.0;
	int sampleCount;

	CLFO voiceEnvLfo[7];

	bool killIt = false;

	float* globalEnv1Ptr;
	float* globalEnv2Ptr;
	float* globalEnv3Ptr;
	float* globalLfo1Ptr;
	float* globalLfo2Ptr;
	float* globalLfo3Ptr;

	

	
	float* envBank[7][2];

	int modChoice = 0;
	float envInc;
	bool active = false;

	int voiceId;
	int noteIdx;
	int32 noteID;
	int generator;

	
	float* mainCEnvPtr;
	float* whichEnv[2];

	int isSustained = 0;

	int decayReleaseToggle = 0;
	int attackFlagToggle = 0;
	float o2 = 0.0;
	float o1 = 0.0;
	float o;
	float env;  //carrier envelope
	float mlev; //modulator target level
	float car;  //carrier oscillator
	float dcar;
	float cenv; //smoothed env
	float catt; //smoothing
	float attCP;
	float cdec; //carrier envelope decay
	int postAttack = 1;
	int attackDone = 1;
	int32 note; //remember what note triggered this
	float velocity;
	int32 vel;
	float prevO = 0.0f;
	bool rampingUp;
	bool rampingDown;
	float l;

	float E1env;
	float E1cenv;
	float E1catt;
	float E1cdec;
	float E1envInc;


	float oEnvInc;
	float EOffEnv;
	float EOffCenv;
	float EOffCatt;
	float EOffCdec;
	float EOffenvInc;

	float levOutTarget = 0.0;
	float levOut = 0.0;
	float levOutInc = 0.0;
	float levOutIntermediate = 0.0;
	float adjustedRatio;
};



