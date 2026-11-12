#ifndef __CONSTANTS__
#define __CONSTANTS__
#include "Env.h"
/*
#if SMTG_OS_WINDOWS && !defined(__GNUC__)
typedef long int32;
typedef unsigned long uint32;
#else
typedef int int32;
typedef unsigned int uint32;
#endif
*/

#ifndef __PARTPARAMS__
#define __PARTPARAMS__
const int PARTPARAMS = 50;
#endif

#ifndef __FLOATPARAMS__
#define __FLOATPARAMS__
const int FLOATPARAMS = 50;
#endif

#ifndef __NPARAMS__
#define __NPARAMS__
const int NPARAMS = 100;
#endif

#ifndef __GPARAMS__
#define __GPARAMS__
const int GPARAMS = 1000;
#endif

#ifndef __PARTVOICES__
#define __PARTVOICES__
const int PARTVOICES = 128;
#endif

#ifndef __GENCOUNT__
#define __GENCOUNT__
const int GENCOUNT = 1;
#endif

#ifndef __GENRAMPEDPARAMSLENGTH__
const int  GENRAMPEDPARAMSLENGTH = 6;
#endif

#ifndef __PROCRAMPEDPARAMSLENGTH__
const int  PROCRAMPEDPARAMSLENGTH = 1;
#endif

#ifndef __ANIM_COUNT__
#define __ANIM_COUNT__
const int ANIMCOUNT = 20;
#endif

#ifndef __RESETPARAMCOUNT__
#define __RESETPARAMCOUNT__
const int RESETPARAMCOUNT = 7;
#endif

#ifndef __ENV_PARAMS__
#define __ENV_PARAMS__
const int ENV_PARAMS = 41;
#endif

#ifndef __ENV_COUNT__
#define __ENV_COUNT__
const int ENV_COUNT = 7;
#endif

#ifndef ___GEN_STEP_PARAM_COUNT___
#define ___GEN_STEP_PARAM_COUNT__
const int _GEN_STEP_PARAM_COUNT_ = 22;
#endif

#ifndef __PI__
#define __PI__
constexpr double PI = 3.14159265358979323846;
#endif

class constants {
public:
	constants() {};
	~constants() {};
};
#endif


#ifndef __MAIN_ENV_ATTACK__
#define __MAIN_ENV_ATTACK__
const int MAIN_ENV_ATTACK = 100;
#endif

#ifndef __MAIN_ENV_DECAY__
#define __MAIN_ENV_DECAY__
const int MAIN_ENV_DECAY = 101;
#endif

#ifndef __MAIN_ENV_RELEASE__
#define __MAIN_ENV_RELEASE__
const int MAIN_ENV_RELEASE = 102;
#endif



#ifndef __VOICE_ENV_ATTACK__
#define __VOICE_ENV_ATTACK__
const int VOICE_ENV_ATTACK = 105;
#endif

#ifndef __VOICE_ENV_DECAY__
#define __VOICE_ENV_DECAY__
const int VOICE_ENV_DECAY = 106;
#endif

#ifndef __VOICE_ENV_RELEASE__
#define __VOICE_ENV_RELEASE__
const int VOICE_ENV_RELEASE = 107;
#endif


#ifndef __SYNC_ATTACK__
#define __SYNC_ATTACK__
const int SYNC_ATTACK = 200;
#endif
#ifndef __SYNC_DEC__
#define __SYNC_DEC__
const int SYNC_DEC = 201;
#endif
#ifndef __SYNC_REL__
#define __SYNC_REL__
const int SYNC_REL = 202;
#endif
#ifndef __SYNC_TUNE__
#define __SYNC_TUNE__
const int SYNC_TUNE = 203;
#endif
#ifndef __SYNC_FINE__
#define __SYNC_FINE__
const int SYNC_FINE = 204;
#endif
#ifndef __SYNC_OCT__
#define __SYNC_OCT__
const int SYNC_OCT = 205;
#endif
#ifndef __SYNC_INT__
#define __SYNC_INT__
const int SYNC_INT = 206;
#endif
#ifndef __SYNC_MODGAIN__
#define __SYNC_MODGAIN__
const int SYNC_MODGAIN = 207;
#endif
#ifndef __SYNC_MODOCT__
#define __SYNC_MODOCT__
const int SYNC_MODOCT = 208;
#endif
#ifndef __SYNC_MODINT__
#define __SYNC_MODINT__
const int SYNC_MODINT = 209;
#endif
#ifndef __SYNC_MODTUNE__
#define __SYNC_MODTUNE__
const int SYNC_MODTUNE = 210;
#endif
#ifndef __SYNC_MODFINE__
#define __SYNC_MODFINE__
const int SYNC_MODFINE = 211;
#endif
#ifndef __SYNC_GAIN__
#define __SYNC_GAIN__
const int SYNC_GAIN = 212;
#endif
#ifndef __SYNC_MODFREAQBEAT__
#define __SYNC_MODFREAQBEAT__
const int SYNC_MODFREAQBEAT = 213;
#endif

//need 10 switch global buttons per generator == 20
//1-2 == phase 
//3-4 == pitch
//5-6 == modgain
//7-8 == modpitch
//9-10 == maingain



#ifndef __paramEnums__
#define __paramEnums__

const int OSC_ = 0;
const int WIDTH_ = 1;
const int WIDTH_ENV = 2;
const int WIDTH_ENV_SRC = 3;
const int TUNE_ = 4;
const int TUNE_ENV = 5;
const int TUNE_ENV_SRC = 6;
const int FINE_ = 7;
const int FINE_ENV = 8;
const int FINE_ENV_SRC = 9;
const int OCT_ = 10;
const int OCT_ENV = 11;
const int OCT_ENV_SRC = 12;
const int INT_ = 13;
const int INT_ENV = 14;
const int INT_ENV_SRC = 15;
const int MOD_OSC_ = 16;
const int MOD_TYPE_ = 17;
const int MOD_GAIN_ = 18;
const int MOD_GAIN_ENV = 19;
const int MOD_GAIN_ENV_SRC = 20;
const int MOD_WIDTH_ = 21;
const int MOD_WIDTH_ENV = 22;
const int MOD_WIDTH_ENV_SRC = 23;
const int MOD_TUNE_ = 24;
const int MOD_TUNE_ENV = 25;
const int MOD_TUNE_ENV_SRC = 26;
const int MOD_FINE_ = 27;
const int MOD_FINE_ENV = 28;
const int MOD_FINE_ENV_SRC = 29;
const int MOD_OCT_ = 30;
const int MOD_OCT_ENV = 31;
const int MOD_OCT_ENV_SRC = 32;
const int MOD_INT_ = 33;
const int MOD_INT_ENV = 34;
const int MOD_INT_ENV_SRC = 35;
const int MOD_BEAT_ = 36;
const int MOD_BEAT_ENV = 37;
const int MOD_BEAT_ENV_SRC = 38;
const int GAIN_ = 39;
const int GAIN_ENV = 40;
const int GAIN_ENV_SRC = 41;
const int FILTER_ = 42;
const int FILTER_ENV = 43;
const int FILTER_ENV_SRC = 44;
const int TRI_WIDTH = 45;
const int MOD_TRI_WIDTH = 46;
const int FILTER_Q = 47;
const int AUX_10 = 48;
const int AUX_11 = 49;
const int AUX_12 = 50;
const int MOD_OSC_GAIN = 51;

const int ENV_1_ATTACK = 110;
const int ENV_1_DECAY = 111;
const int ENV_1_RELEASE = 112;
const int ENV_1_DECAYTOGGLE = 113;

const int ENV_1_GLOBAL = 114;

const int ENV_2_ATTACK = 115;

const int ENV_2_DECAY = 116;

const int ENV_2_RELEASE = 117;

const int ENV_2_DECAYTOGGLE = 118;

const int ENV_2_GLOBAL = 119;

const int ENV_3_ATTACK = 120;

const int ENV_3_DECAY = 121;

const int ENV_3_RELEASE = 122;

const int ENV_3_DECAYTOGGLE = 123;

const int ENV_3_GLOBAL = 124;

const int LFO_1_RATE = 125;

const int LFO_1_AMP = 126;

const int LFO_1_TYPE = 127;

const int LFO_1_BPM = 128;

const int LFO_1_GLOBAL = 129;

const int LFO_2_RATE = 130;

const int LFO_2_AMP = 131;

const int LFO_2_TYPE = 132;

const int LFO_2_BPM = 133;

const int LFO_2_GLOBAL = 134;

const int LFO_3_RATE = 135;

const int LFO_3_AMP = 136;

const int LFO_3_TYPE = 137;

const int LFO_3_BPM = 138;

const int LFO_3_GLOBAL = 139;

const int LFO_1_PHASE = 140;
const int LFO_2_PHASE = 141;
const int LFO_3_PHASE = 142;

//

const int WIDTH_RESET = 150;
const int TUNE_RESET = 151;
const int FINE_RESET = 152;
const int MOD_GAIN_RESET = 153;
const int MOD_WIDTH_RESET = 154;
const int MOD_TUNE_RESET = 155;
const int MOD_FINE_RESET = 156;

const int TOGGLE_GEN_VIEW = 157;

const int WIDTH_ENV_ADD_MULT = 160;
const int TUNE_ENV_ADD_MULT = 161;
const int FINE_ENV_ADD_MULT = 162;
const int OCT_ENV_ADD_MULT = 163;
const int INT_ENV_ADD_MULT = 164;

const int MOD_GAIN_ENV_ADD_MULT = 165;
const int MOD_WIDTH_ENV_ADD_MULT = 166;
const int MOD_TUNE_ENV_ADD_MULT = 167;
const int MOD_FINE_ENV_ADD_MULT = 168;
const int MOD_OCT_ENV_ADD_MULT = 169;
const int MOD_INT_ENV_ADD_MULT = 170;
const int MOD_BEAT_ENV_ADD_MULT = 171;

const int MAIN_GAIN_ENV_ADD_MULT = 171;
const int MAIN_FILTER_ENV_ADD_MULT = 172;

const int GAINER = 200;


const int WIDTH_RAMPINDEX = 0;
const int GAIN_RAMPINDEX = 1;
const int FILTER_RAMPINDEX = 2;
const int MOD_GAIN_RAMPINDEX = 3;
const int MOD_WIDTH_RAMPINDEX = 4;
const int MOD_OSC_GAIN_RAMPINDEX = 5;

const  float quants[11] = {
					240,
					180,
					120,
					90,
					60,
					30,
					20,
					15,
					10,
					7.5,
					3.75 };

#endif

//IDXNAME, typestr, ustrlength