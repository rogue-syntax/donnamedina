

#ifndef __CONSTANTS__
#define __CONSTANTS__
#include "Env.h"
#include <string>
#include <cmath>
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
#include "version.h"  // the one version number: host, About screen and update check (2026-11-20)
const char* const RSVERSION = "Version " VERSION_STR; // const pointer: internal linkage, one copy per TU is fine
// The trademark sign (U+2122) in UTF-8, for text the plugin draws: the store's name rule
// (docs/store/PLUGIN-DEVELOPER-CHECKLIST.md step 5) is "Donna Medina" TM_SIGN and
// "Rogue Syntax Sound Design" TM_SIGN. A separate literal, so the \xA2 can't run into the next letter.
#define TM_SIGN "\xE2\x84\xA2"
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

// Scope (2026-11-14): processor -> controller, one frame (binary "data", ScopeCapture::kLength floats),
// sent by the processor's main-thread timer; controller -> processor, int "on" = editor open/closed.
const char PMSG_SCOPE_CHAR = 'd';
const char CMSG_SCOPE_ENABLE_CHAR = 'e';
// Voice strip (2026-11-14): processor -> controller, binary "data" = 128 bytes, one per pitch:
// bit 7 = key held, bits 0..6 = amp-envelope level (0 = silent, 1..127). Sent by the same timer when
// the bytes change, also only while an editor is open. voiceStateByte() is the encoding.
const char PMSG_VOICES_CHAR = 'f';
// Output peak (2026-11-14): processor -> controller, float "peak" = the largest |sample| of either
// output channel since the previous message (linear, 1.0 = 0 dBFS). Sent by the same timer while an
// editor is open; one 0 is sent when the output goes silent, then nothing until it sounds again.
const char PMSG_PEAK_CHAR = 'g';
// Preset bar (2026-11-14): controller -> processor, binary "data" = a processor state (the
// BaseProcessor::getState layout); the processor applies it with setState (main thread).
const char CMSG_SET_STATE_CHAR = 'h';
// controller -> processor, int "purpose": send the current state back (PMSG_STATE_CHAR, same purpose).
const char CMSG_GET_STATE_CHAR = 'i';
const char PMSG_STATE_CHAR = 'j';
// Playheads (2026-11-15): processor -> controller, binary "data" = kPlayheadSlots PlayheadSlot, the
// most recently started voices, newest first (pitch -1 = empty slot). Sent by the timer every tick
// while any slot is used, then one all-empty message; only while an editor is open.
const char PMSG_PLAYHEADS_CHAR = 'k';
// Licensing v1 (docs/07-licensing.md): controller -> processor after a license was pasted and
// stored. Carries nothing; the processor re-reads and verifies the stored token itself.
const char CMSG_LICENSE_CHANGED_CHAR = 'l';
const int kPlayheadSlots = 4;
struct PlayheadSlot {
	int pitch;     // -1 = empty
	float age;     // seconds since the voice's envelopes and LFOs (re)started
	float relAge;  // seconds since its release started, -1 while the key is held
	float level;   // amp-envelope level (env 1 cenv)
	float src[6];  // the voice's modulation sources 1..6 (env 1..3, LFO 1..3 output; an LFO nothing
	               // is routed to is not computed, so its value is stale)
};
// Default voice limit (DXGenerator::POLYCOUNT starts here); the POLY_COUNT parameter sets it.
const int POLY_LIMIT = 32;
inline unsigned char voiceStateByte (bool held, float level) {
	if (!(level > 0.0001f)) return 0;  // silent (the engine drops voices below -80 dB)
	if (level > 1.0f) level = 1.0f;
	int l = static_cast<int> (level * 127.0f + 0.999f);  // any audible voice is at least 1
	if (l > 127) l = 127;
	return static_cast<unsigned char> ((held ? 0x80 : 0) | l);
}


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

// Master output volume (2026-11-14). Global: not part of the presets (a preset change keeps it) and
// saved in a tagged block after the bypass flag (see BaseProcessor::getState), because older states
// hold 0 in this slot of the parameter array. 144/145 are left free: vstgui.uidesc still has old
// control-tag names on them.
const int MASTER_VOL = 146;
// Squared taper: gain = 2 v^2, so 0 = silence, 0.7071 = 0 dB (default), 1 = +6 dB.
const double DEFAULT_MASTER_VOL = 0.70710678118654752;
inline double masterVolGain (double v) {
	if (!(v > 0.0)) return 0.0;  // also catches NaN
	if (v > 1.0) v = 1.0;
	return 2.0 * v * v;
}
const unsigned int MASTER_VOL_STATE_TAG = 0x4C4F564Du;  // "MVOL" (little endian)

// Polyphony (2026-11-15): how many voices may sound at once, 1..POLY_MAX (the oldest released or
// quietest voice is stolen beyond it, DXGenerator::noteOn). Global like MASTER_VOL: not part of the
// presets, saved in a tagged block after the master volume ("POLY"); states without it load at
// POLY_DEFAULT. Host parameter: 64 steps, normalized (n - 1) / 63.
const int POLY_COUNT = 147;
const int POLY_MAX = 64;
const int POLY_DEFAULT = 32;
inline int polyFromNormalized (double v) {
	if (!(v > 0.0)) return 1;  // also catches NaN
	if (v > 1.0) v = 1.0;
	return 1 + static_cast<int> (v * (POLY_MAX - 1) + 0.5);
}
inline double polyToNormalized (int n) {
	n = n < 1 ? 1 : (n > POLY_MAX ? POLY_MAX : n);
	return double (n - 1) / double (POLY_MAX - 1);
}
const unsigned int POLY_STATE_TAG = 0x594C4F50u;  // "POLY" (little endian)

// HQ mode (2026-11-15): the voices run at twice the host rate and are filtered back down
// (hqDecimator.h), which reduces FM aliasing on high notes at about twice the voice CPU. Global like
// POLY_COUNT (not in presets, tagged block "HQ  "). Switching cuts the sounding notes (the voices are
// rebuilt for the new rate at the start of the next audio chunk).
const int HQ_MODE = 109;
const unsigned int HQ_STATE_TAG = 0x20205148u;  // "HQ  " (little endian)

// FX strip (2026-11-15, fxStrip.h): chorus and a tempo-synced ping-pong delay, part of the preset.
// Ids above the step buttons (173..184), so isHostParam() includes this block.
const int DLY_TIME = 191;    // tempo division list (fx::delayBeats)
const int DLY_FDBK = 192;
const int DLY_TONE = 193;
const int DLY_MIX = 194;
const int CHO_RATE = 195;
const int CHO_DEPTH = 196;
const int CHO_MIX = 197;
const int FX_PARAM_FIRST = DLY_TIME, FX_PARAM_LAST = CHO_MIX;

// Global parameters: kept across preset changes, saved in their own tagged state blocks.
inline bool isGlobalParam (int pid) { return pid == 146 /* MASTER_VOL */ || pid == POLY_COUNT || pid == HQ_MODE; }

// Snap (2026-11-15): the retrigger/steal fade (a sounding voice fades out before it restarts or is
// taken) was a fixed 21 ms. SNAP_TIME sets it, 2..80 ms on a log curve (ms = 2 * 40^v); part of the
// preset. Short = a hard, clicky re-attack; long = smooth repeats.
const int SNAP_TIME = 148;
inline double snapMs (double v) {
	if (!(v > 0.0)) v = 0.0;
	if (v > 1.0) v = 1.0;
	return 2.0 * std::pow (40.0, v);
}
inline double snapToNormalized (double ms) { return std::log (ms / 2.0) / std::log (40.0); }
const double SNAP_DEFAULT_MS = 21.0;

// OSC KEY SYNC (2026-11-17), what a repeated pitch (the voice is still sounding) does:
// off (default) = re-attack the running voice at once: envelopes from their current level,
// oscillators, LFOs, filter and drift keep running (no delay, nothing jumps);
// on = fade out over SNAP_TIME, then restart from a reset voice (oscillator phase, LFOs, filter):
// the same attack every time, after the fade. Part of the preset. Id 103 was free (below the step
// buttons, so it is a host parameter and saved in presets).
const int KEY_SYNC = 103;

// Drift (2026-11-15): each note (re)start picks small random offsets, scaled by DRIFT_AMOUNT (0 = off):
// pitch up to +-kDriftCents (both oscillators, so the FM ratio stays), filter cutoff up to
// +-kDriftFilter (knob units) and pan up to +-kDriftPan. Triangular distribution (most notes close to
// centre). Part of the preset.
const int DRIFT_AMOUNT = 108;
const float kDriftCents = 15.0f, kDriftFilter = 0.06f, kDriftPan = 0.3f;

// Expression (2026-11-15): one bipolar knob that offsets the sound's brightness. EXPR_AMOUNT (centre =
// none) moves the destination(s) EXPR_TARGET picks; with EXPR_VEL on, each note's velocity scales it
// (the knob is then the velocity depth; left = harder notes darker). Offsets are in knob units at
// full amount (kExprDepth). Part of the preset.
const int EXPR_AMOUNT = 149;
const int EXPR_VEL = 158;
const int EXPR_TARGET = 159;
enum ExprTarget { kExprBright = 0, kExprFilter, kExprFmDepth, kExprWidth, kExprTargetCount };
inline const char* exprTargetName (int t) {
	static const char* const k[kExprTargetCount] = {"BRIGHT", "FILTER", "FM DEPTH", "WIDTH"};
	return (t >= 0 && t < kExprTargetCount) ? k[t] : "";
}
inline int exprTargetFromNormalized (double v) {
	int t = static_cast<int> (v * (kExprTargetCount - 1) + 0.5);
	return t < 0 ? 0 : (t >= kExprTargetCount ? kExprTargetCount - 1 : t);
}
// depth per destination at full amount, knob units: filter cutoff, FM depth (MOD GAIN), osc widths
inline void exprDepths (int target, float& filter, float& fm, float& width) {
	filter = fm = width = 0.0f;
	switch (target) {
	case kExprBright: filter = 0.35f; fm = 0.35f; break;
	case kExprFilter: filter = 0.5f; break;
	case kExprFmDepth: fm = 0.5f; break;
	case kExprWidth: width = 0.4f; break;
	default: break;
	}
}

// State version (2026-11-15). Parameters added after the first release would load as 0 from older
// states and presets (their slot in the saved array was unused). getState now ends with a tagged
// version block; when a state's version is older than the one a parameter arrived in, that
// parameter gets its default (applyNewParamDefaults). Readers take the tagged blocks after the
// bypass flag in any order: MASTER_VOL_STATE_TAG, POLY_STATE_TAG, STATE_VERSION_TAG.
const unsigned int STATE_VERSION_TAG = 0x52455653u;  // "SVER" (little endian)
const unsigned int STATE_VERSION = 3;                 // 1 = states without the block; 3: KEY_SYNC
struct NewParamDefault { int pid; double normalized; unsigned int since; };
inline const NewParamDefault* newParamDefaults (int& count) {
	static const NewParamDefault k[] = {
		{SNAP_TIME, snapToNormalized (SNAP_DEFAULT_MS), 2},
		{EXPR_AMOUNT, 0.5, 2},
		{EXPR_VEL, 0.0, 2},
		{EXPR_TARGET, 0.0, 2},
		{DRIFT_AMOUNT, 0.0, 2},
		{DLY_TIME, 7.0 / 11.0, 2},  // 1/8.
		{DLY_FDBK, 0.35, 2},
		{DLY_TONE, 0.6, 2},
		{DLY_MIX, 0.0, 2},
		{CHO_RATE, 0.4, 2},
		{CHO_DEPTH, 0.5, 2},
		{CHO_MIX, 0.0, 2},
		{KEY_SYNC, 0.0, 3},
	};
	count = static_cast<int> (sizeof (k) / sizeof (k[0]));
	return k;
}
// Calls set(pid, normalized) for every parameter newer than `version`.
template <typename F>
inline void applyNewParamDefaults (unsigned int version, F set) {
	int n = 0;
	const NewParamDefault* d = newParamDefaults (n);
	for (int i = 0; i < n; i++)
		if (d[i].since > version)
			set (d[i].pid, d[i].normalized);
}

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
// B-07: tags from here on are the momentary step-control buttons (GUI only: they are not stored as
// values or edited as host parameters). Replaces the magic number 173 in the controllers.
const int STEP_BUTTONS_FIRST = STEP_CTRL_1;
// Host (automatable) parameters: everything below the step buttons, and the FX block above them.
inline bool isHostParam (int tag) { return tag < STEP_BUTTONS_FIRST || (tag >= FX_PARAM_FIRST && tag <= FX_PARAM_LAST); }

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