

#ifndef __CONSTANTS__
#define __CONSTANTS__
#include "Env.h"
#include <string>
/*
#if SMTG_OS_WINDOWS && !defined(__GNUC__)
typedef long int32;
typedef unsigned long uint32;
#else
typedef int int32;
typedef unsigned int uint32;
#endif
*/

#ifndef __RSVERSION__
#define __RSVERSION__
const char* const RSVERSION = "Version 1.0.1"; // const pointer: internal linkage, one copy per TU is fine
#endif

//
#ifndef __PARTPARAMS__
#define __PARTPARAMS__
const int PARTPARAMS = 55;
#endif



#ifndef __FLOATPARAMS__
#define __FLOATPARAMS__
const int FLOATPARAMS = 55;
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
const int  GENRAMPEDPARAMSLENGTH = 8;
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
//const int ENV_PARAMS = 54;
const int ENV_PARAMS = 52;
#endif

#ifndef __ENV_COUNT__
#define __ENV_COUNT__
const int ENV_COUNT = 7;
#endif

#ifndef ___GEN_STEP_PARAM_COUNT___
#define ___GEN_STEP_PARAM_COUNT__
const int _GEN_STEP_PARAM_COUNT_ = 23;
#endif

#ifndef __PI__
#define __PI__
constexpr double PI = 3.14159265358979323846;
#endif



class constants {
public:
	constants() {};
	~constants() {};

	static bool isRegLegit;

	//static char* constants::NVGParamDef[PARTPARAMS];



};
#endif



/*
DONT FORGET TO ADD PARAM CONT INTS TO CONTROLLER.H (MDADX10CONTROLLER)
SWITCH PARAMS NEED TO BE ADDED TO mdadx10controller vertSwitchPids[i] as well
*/

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

// B-27: message IDs below are single chars; IMessage::setMessageID needs a NUL-terminated string.
// msgIdStr(X_CHAR) returns "X" (static storage). Receivers only look at the first char.
inline const char* msgIdStr(char c) {
	static const struct Table {
		char s[256][2];
		Table() { for (int i = 0; i < 256; i++) { s[i][0] = static_cast<char>(i); s[i][1] = 0; } }
	} table;
	return table.s[static_cast<unsigned char>(c)];
}

const int CMSG_GET_SAMPLE_RATE = 0;
const char CMSG_GET_SAMPLE_RATE_CHAR = '0';
//const int CMSG_GET_SAMPLE_RATE = int(CMSG_GET_SAMPLE_RATE_CHAR);

const int CMSG_LINK_TUNE = 1;
const char CMSG_LINK_TUNE_CHAR = '1';

const int CMSG_STEP_SWITCH = 2;
const char CMSG_STEP_SWITCH_CHAR = '2';

const int PMSG_STEP_SWITCH = 3;
const char PMSG_STEP_SWITCH_CHAR = '3';

const int PMSG_LFO4_RATE = 4;
const char PMSG_LFO4_RATE_CHAR = '4';

const int PMSG_LFO5_RATE = 5;
const char PMSG_LFO5_RATE_CHAR = '5';

const int PMSG_LFO6_RATE = 6;
const char PMSG_LFO6_RATE_CHAR = '6';

const int PMSG_LFO_RATE = 7;
const char PMSG_LFO_RATE_CHAR = '7';

const int PMSG_SET_PARAM_NRML = 8;
const char PMSG_SET_PARAM_NRML_CHAR = '8';

const int PMSG_BADREG = 9;
const char PMSG_BADREG_CHAR = '9';

const int PMSG_STEP_SWITCH2 = 10;
const char PMSG_STEP_SWITCH2_CHAR = 'a';

const int PMSG_CTRLR_READY = 11;
const char PMSG_CTRLR_READY_CHAR = 'b';

// B-53: was the multi-character literal 'sc', which MSVC truncates to its last char 'c'. Same value,
// now explicit. The unused PMSG_STEP_CTRL_1..12_CHAR ('s1'..'sB') were removed: they truncated to
// '1'..'9', '0', 'A', 'B' and so collided with other IDs (e.g. 's1' == CMSG_LINK_TUNE_CHAR '1').
// Message IDs are single chars; the receiver switches on the first char only.
const char PMSG_STEP_CTRL_CHAR = 'c';


const char CONNECT_ID = 'CID';


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
//const int TRI_WIDTH = 45;
//const int MOD_TRI_WIDTH = 46;
const int FILTER_Q = 45;
const int AUX_10 = 46;
const int AUX_11 = 47;
const int AUX_12 = 48;
const int MOD_OSC_GAIN = 49;
const int SYNC_GEN_MOD_TUNE = 50;
const int SYNC_GEN_MOD_FINE = 51;

const int PAN_LR = 52;
const int PAN_LR_ENV = 53;
const int PAN_LR_ENV_SRC = 54;

const int ENV_1_ATTACK = 110;
const int ENV_1_DECAY = 111;
const int ENV_1_RELEASE = 112;
const int ENV_1_DECAYTOGGLE = 113;

const int ENV_1_DELAY = 114;

const int ENV_2_ATTACK = 115;

const int ENV_2_DECAY = 116;

const int ENV_2_RELEASE = 117;

const int ENV_2_DECAYTOGGLE = 118;

const int ENV_2_DELAY = 119;

const int ENV_3_ATTACK = 120;

const int ENV_3_DECAY = 121;

const int ENV_3_RELEASE = 122;

const int ENV_3_DECAYTOGGLE = 123;

const int ENV_3_DELAY = 124;

const int LFO_1_RATE = 125;

const int LFO_1_AMP = 126;

const int LFO_1_TYPE = 127;

const int LFO_1_BPM = 128;

const int LFO_1_DELAY = 129;

const int LFO_2_RATE = 130;

const int LFO_2_AMP = 131;

const int LFO_2_TYPE = 132;

const int LFO_2_BPM = 133;

const int LFO_2_DELAY = 134;

const int LFO_3_RATE = 135;

const int LFO_3_AMP = 136;

const int LFO_3_TYPE = 137;

const int LFO_3_BPM = 138;

const int LFO_3_DELAY = 139;

const int LFO_1_PHASE = 140;
const int LFO_2_PHASE = 141;
const int LFO_3_PHASE = 142;

const int STEP_CTRL_TARG = 143;

//const int SYNC_MOD_TUNE_MAIN = 143;

//const int SYNC_MOD_TUNE_FINE = 143;

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

const int STEP_CTRL_1 = 173;
const int STEP_CTRL_2 = 174;
const int STEP_CTRL_3 = 175;
const int STEP_CTRL_4 = 176;
const int STEP_CTRL_5 = 177;
const int STEP_CTRL_6 = 178;
const int STEP_CTRL_7 = 179;
const int STEP_CTRL_8 = 180;
const int STEP_CTRL_9 = 181;
const int STEP_CTRL_10 = 182;
const int STEP_CTRL_11 = 183;
const int STEP_CTRL_12 = 184;

const int FINAL_PAN = 190;

const int GAINER = 200;


const int WIDTH_RAMPINDEX = 0;
const int GAIN_RAMPINDEX = 1;
const int FILTER_RAMPINDEX = 2;
const int MOD_GAIN_RAMPINDEX = 3;
const int MOD_WIDTH_RAMPINDEX = 4;
const int MOD_OSC_GAIN_RAMPINDEX = 5;
const int PAN_LR_RAMPINDEX = 6;
const int PAN_LR_ENV_RAMPINDEX = 7;

const float LFO_DELAY_TIME_MIN = 0.03f;


const int MORE_INFO_BUTTON_VIEW_IDX = 901;
const int SHUT_WIN_BUTTON_VIEW_IDX = 902;

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

/*(const  float quants[10] = {
					240,
					180,
					120,
					90,
					60,
					30,
					20,
					15,
					10,
					7.5};*/


const int vertSwitchPids[_GEN_STEP_PARAM_COUNT_] = { OSC_, WIDTH_ENV_SRC, TUNE_ENV_SRC, FINE_ENV_SRC, OCT_, OCT_ENV_SRC, INT_, INT_ENV_SRC, MOD_OSC_, MOD_TYPE_, MOD_GAIN_ENV_SRC, MOD_WIDTH_ENV_SRC, MOD_TUNE_ENV_SRC, MOD_FINE_ENV_SRC, MOD_OCT_, MOD_OCT_ENV_SRC, MOD_INT_, MOD_INT_ENV_SRC, MOD_BEAT_, MOD_BEAT_ENV_SRC, GAIN_ENV_SRC, FILTER_ENV_SRC, PAN_LR_ENV_SRC };

const int TOGGLE_COUNT = 7;

const int toggleBtnPids[TOGGLE_COUNT] = { SYNC_GEN_MOD_TUNE,
ENV_1_DECAYTOGGLE,
ENV_2_DECAYTOGGLE,
ENV_3_DECAYTOGGLE,
LFO_1_BPM,
LFO_2_BPM,
LFO_3_BPM };

#endif

//IDXNAME, typestr, ustrlength