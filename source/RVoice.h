
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
	//CWTOscillatorVoice WTOsc2;
	CQBLimitedOscillator WTOsc2;
	CQBLimitedOscillator WTOsc;
	float portoInc = 0.0f;
	float targOscFo = 0.0f;
	float mergedOscFo = 0.0f;
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
	int sampleCount = 0;

	CLFO voiceEnvLfo[7];

	float osc2out = 0.0f;
	float osc1pure = 0.0f;

	float* globalEnv1Ptr = nullptr;
	float* globalEnv2Ptr = nullptr;
	float* globalEnv3Ptr = nullptr;
	float* globalLfo1Ptr = nullptr;
	float* globalLfo2Ptr = nullptr;
	float* globalLfo3Ptr = nullptr;

	float widthPole = 0.0f;
	float modWidthPole = 0.0f;

	bool noteActive = false;
	
	//float* envBank[7][2];
	float* envBank[7];

	int modChoice = 0;
	float envInc = 0.0f;
	bool active = false;

	int voiceId = 0;
	int noteIdx = 0;
	int32 noteID = 0;
	int generator = 0;

	
	float* mainCEnvPtr = nullptr;
	float* whichEnv[2];

	int isSustained = 0;
	float prePan = 0.0f;
	int decayReleaseToggle = 0;
	int attackFlagToggle = 0;
	float o2 = 0.0;
	float o1 = 0.0;
	float o = 0.0;
	//float env;  //carrier envelope
	//float mlev; //modulator target level
	float car;  //carrier oscillator
	float dcar;
	//float cenv; //smoothed env
	//float catt; //smoothing
	//float attCP;
	//float cdec; //carrier envelope decay*/
	//int postAttack = 1;
	//int attackDone = 1;
	int32 note = 0; //remember what note triggered this
	float velocity = 0.0f;
	int32 vel = 0;
	//float prevO = 0.0f;
	bool rampingUp = true;
	bool rampingDown = false;
	//float l = 0.0f;
	float pan = 0.0;
	//float E1env;
	float E1cenv = 0.0;
	//float E1catt;
	//float E1cdec;
	//float E1envInc;

	/*
	float oEnvInc = 0.0;
	float EOffEnv = 0.0;
	float EOffCenv = 0.0;
	float EOffCatt;
	float EOffCdec;
	float EOffenvInc;
	*/
	//float levOutTarget = 0.0;
	//float levOut = 0.0;
	//float levOutInc = 0.0f;
	//float levOutIntermediate = 0.0f;
	//float adjustedRatio;
	bool killIt = false;
	// B-18: a note-off that arrives during the retrigger fade (rampingDown) is held here
	// and applied right after the fade restarts the note, instead of being erased.
	bool releasePending = false;
	float e = 0.0f;
	//float tuneC2;
	int newInt = 0;
	int newOct = 0;
	int newIntEnv = 0;
	int newModOctEnv = 0;
	int newModIntEnv = 0;
	int tuneCEnv = 0;
	int fineCEnv = 0;
	int modTuneCEnv = 0;
	int modFineCEnv = 0;
	int newOctEnv = 0;
	int newModOct = 0;
	int newModInt = 0;
	int squareEnvSrc = 0;
	int modTriEnvSrc = 0;
	int modSquareEnvSrc = 0;
	int triEnvSrc = 0;
	int newIntCtrl = 0;
	int newOctCtrl = 0;
	int newModOctCtrl = 0;
	int newModIntCtrl = 0;
	int newNote;
	float gainer = 0.0;
	float clipper = 1.0f;
	float tuneC = 0.0;
	float tuneM = 0.0;
	float gainerTemp = 0.0;
	float filterEnv = 0.0;
	float filterEnvSrc = 0.0f;
	int filterSrc = 0.0f;
	int filterAddMult = 1;
	int filterAddMultCtrl = 1;
	/*
	float* intEnvPtr;
	float* octEnvPtr;
	float* tuneEnvPtr;
	float* fineEnvPtr;
	float* modIntEnvPtr;
	float* modOctEnvPtr;
	float* modTuneEnvPtr;
	float* modFineEnvPtr;
	float* triEnvPtr;
	float* panEnvPtr;
	float* modTriEnvPtr;
	float* modGainEnvPtr;
	float* gainEnvPtr;
	float* filtEnvPtr;
	*/
};



