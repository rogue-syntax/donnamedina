
#ifndef __event__
#define __event__
#include "constants.h"


#if SMTG_OS_WINDOWS && !defined(__GNUC__)
typedef long int32;
typedef unsigned long uint32;
#else
typedef int int32;
typedef unsigned int uint32;
#endif

#ifndef __PARTVOICES__
#define __PARTVOICES__
const int PARTVOICES = 4;
#endif

#ifndef __GENCOUNT__
#define __GENCOUNT__
const int GENCOUNT = 4;
#endif




#pragma once
#include <string>
#include <array>


class REvent {
public:

	std::string eventName;
	REvent(std::string eventNameIn) : eventName(eventNameIn) {};
	
};

class RData {
public:
	//RData() {};
	std::string dataName;
	RData(std::string eventNameIn) : dataName(eventNameIn) {};

};

class VoiceObj {
public:

	
	VoiceObj(int idIn) : voiceId(idIn) {
		int voiceIdx = idIn;
	};
	VoiceObj() : voiceId(0) {
		int voiceIdx = 0;
	};
	VoiceObj(const VoiceObj &);
	int voiceId;
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
	int32 note; //remember what note triggered this
	int32 velocity;

	float prevEnv;

	float targEnv;

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


class RVoice {
public:
	//RData() {};
	int id;
	//std::string name;
	int voiceCount;

	RVoice(int idIn, std::string nameIn) : voiceId(idIn){
		voiceCount = PARTVOICES;

	};
	RVoice() : id(0) {
		voiceCount = PARTVOICES;

	}
	float o;
	float origO;
	int voiceId;
	int voiceIdx;
	int generator;
	int envIdx;
	float envInc;
	float mEnvInc;
	int envDiv;
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
	int32 note; //remember what note triggered this
	int32 velocity;
	float savedRel;
	float targRel;
	float prevEnv;
	float targEnv;
	float targMEnv;
	float origEnv;
	float origMEnv;
	bool retrig;
	float dE;

	bool rampingUp;
	bool rampingDown;
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

class RVoices {
public:

	int id;
	std::string name;
	int voiceCount;
	//VoiceObj voices[PARTVOICES];
	RVoice voices[PARTVOICES];
	RVoices(int idIn, std::string nameIn) : id(idIn), name(nameIn) {
		voiceCount = PARTVOICES;
	};
	RVoices() : id(0), name("0") {
		voiceCount = PARTVOICES;

	}


};

#endif

