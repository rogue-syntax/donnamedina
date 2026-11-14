#pragma once
// Readable names and values with units for the CONTROL readout (NVGView::drawCtrlValueOutput).
//
// The conversions follow what the engine does with each normalized value (DXGenerator recalc*/
// singleVoiceProcessing, DX10Processor recalcEnv*/recalcLfo*), not the parameter names: e.g. TUNE is
// a linear frequency ratio (x0..x2), not semitones. See docs/04-gui.md "Readout". No SDK dependency,
// so it can be tested on its own.

#include <string>

namespace paramDisplay {

struct Context {
	// Stepped parameters (StringListParameter): the integer the engine receives, i.e. the
	// parameter's string for this value (some lists start at "1"; see DX10Controller::initialize).
	int step = 0;
	// *_ENV amount parameters: the step of the paired *_ENV_SRC parameter (0 = none), or -1.
	int srcStep = -1;
	// LFO_n_RATE: true when that LFO's BPM sync is on.
	bool lfoSynced = false;
};

// Readable parameter name ("OSC TUNE", "AMP ATTACK", ...); "" for ids without one.
const char* label(int pid);

// Value with units for parameters whose value the readout formats; "" for the others (wave/type/
// source switches and toggles are named by their own NVGView handlers).
std::string value(int pid, double normalized, const Context& ctx);

// The source names used for *_ENV_SRC (index = engine value, 0 = none).
const char* sourceName(int src);

// For a *_ENV amount parameter: the id of its *_ENV_SRC parameter, else -1.
int sourceParamFor(int pid);

// For LFO_n_RATE: the id of that LFO's BPM sync toggle, else -1.
int syncParamFor(int pid);

// MASTER_VOL as dB ("0.0 dB", "-6.0 dB", "-inf dB"); also the host's text for the parameter.
std::string masterVolText(double normalized);

// Voice view in the scope: the pitch range to draw for the voice states (voiceStateByte per pitch,
// 0 = silent). lo..hi spans the lowest..highest sounding pitch, widened to at least 12 semitones
// (centred, kept inside 0..127). With nothing sounding the previous range (lastLo..lastHi) is kept.
void voiceRange(const unsigned char* states, int lastLo, int lastHi, int& lo, int& hi);

// Engine conversions (exposed for tests).
double filterCutoffHz(double normalized);       // DXGenerator filterLogScale2: 80..18000 Hz, 0.5 -> 1800
double envTimeSeconds(int pid, double normalized); // time to cover 95% (3 time constants); <0 = hold
double lfoRateHz(double normalized);             // free-running LFO rate
const char* lfoSyncDivision(double normalized);  // BPM-synced LFO rate (quants[] in constants.h)

} // namespace paramDisplay
