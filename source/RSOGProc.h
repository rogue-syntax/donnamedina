#ifndef __rsogp__
#define __rsogp__

#pragma once
#include "pluginterfaces/vst/vsttypes.h"
#include <string>
#include "REvent.h"
#include "constants.h"
#include "RVoice.h"

class VoiceCo : public RVoice {
public:
	VoiceCo(int idIn, std::string nameIn) : RVoice(idIn, nameIn) {
		int voiceIdx = idIn;
	};
	VoiceCo() : RVoice() {
		int voiceIdx = 0;
	};

	int voiceIdx;
	int generator;

	float env;  //carrier envelope
	float dmod; //modulator oscillator 
	float mod0;
	float mod1;
	float menv; //modulator envelope
	float mlev; //modulator target level
	float mdec; //modulator envelope decay
	float car;  //carrier oscillator
	float dcar;
	float cenv; //smoothed env
	float catt; //smoothing
	float cdec; //carrier envelope decay
	Steinberg::int32 note; //remember what note triggered this
	Steinberg::int32 velocity;

	float prevEnv;

	/*
	float tune;
	float rati;
	float ratf;
	float ratio; //modulator ratio
	float catt;
	float cdec;
	float crel;        //carrier envelope
	float depth;
	float dept2;
	float mdec;
	float mrel; //modulator envelope
	float lfo0;
	float lfo1;
	float dlfo;
	float modwhl;
	float MW;
	float pbend;
	float velsens;
	float volume;
	float vibrato; //LFO and CC
	float rich;
	float modmix;
	


	Steinberg::int32 v;
	Steinberg::int32 vl;
	float l;
	float o;
	float x;
	float e;
	float mw;
	float w;
	float m;
	Steinberg::int32 k;
	*/


};
/*
class VoiceArray {
public:
	//RData() {};
	int genId;
	int voiceCount;
	std::array<VoiceObj, PARTVOICES> voices;
	RVoice(int idIn) : genId(idIn) {
		voiceCount = PARTVOICES;
		VoiceObj vo;
		voices.fill(vo);
	};

};
*/
#endif


