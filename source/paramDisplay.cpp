#include "paramDisplay.h"
#include "constants.h"
#include "lfoShape.h"

#include <cmath>
#include <cstdio>

namespace paramDisplay {

namespace {

std::string fmt (const char* f, double a)
{
	char buf[64];
	std::snprintf (buf, sizeof (buf), f, a);
	return buf;
}

std::string fmt (const char* f, double a, double b)
{
	char buf[64];
	std::snprintf (buf, sizeof (buf), f, a, b);
	return buf;
}

double clamp01 (double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

std::string percent (double v) { return fmt ("%.0f %%", v * 100.0); }
std::string bipolarPercent (double v) { return fmt ("%+.0f %%", (v - 0.5) * 200.0); }

std::string timeText (double seconds)
{
	if (seconds < 0.0)
		return "HOLD";
	if (seconds < 0.01)
		return fmt ("%.1f ms", seconds * 1000.0);
	if (seconds < 1.0)
		return fmt ("%.0f ms", seconds * 1000.0);
	return fmt ("%.2f s", seconds);
}

std::string hzText (double hz)
{
	if (hz >= 1000.0)
		return fmt ("%.2f kHz", hz / 1000.0);
	return fmt ("%.0f Hz", hz);
}

// Frequency ratio -> "x1.250  +3.86 st"
std::string ratioText (double ratio)
{
	if (ratio < 0.001)
		return "x0.000";
	return fmt ("x%.3f  %+.2f st", ratio, 12.0 * std::log2 (ratio));
}

struct Group {
	int base;
	const char* name;
	const char* amt;
	const char* src;
};

// Parameters that come as base / _ENV amount / _ENV_SRC triples (consecutive ids in constants.h).
const Group kGroups[] = {
	{WIDTH_, "OSC WIDTH", "OSC WIDTH MOD", "OSC WIDTH SRC"},
	{TUNE_, "OSC TUNE", "OSC TUNE MOD", "OSC TUNE SRC"},
	{FINE_, "OSC FINE", "OSC FINE MOD", "OSC FINE SRC"},
	{OCT_, "OSC OCTAVE", "OSC OCTAVE MOD", "OSC OCTAVE SRC"},
	{INT_, "OSC INTERVAL", "OSC INTERVAL MOD", "OSC INTERVAL SRC"},
	{MOD_GAIN_, "MOD DEPTH", "MOD DEPTH MOD", "MOD DEPTH SRC"},
	{MOD_WIDTH_, "MOD WIDTH", "MOD WIDTH MOD", "MOD WIDTH SRC"},
	{MOD_TUNE_, "MOD TUNE", "MOD TUNE MOD", "MOD TUNE SRC"},
	{MOD_FINE_, "MOD FINE", "MOD FINE MOD", "MOD FINE SRC"},
	{MOD_OCT_, "MOD OCTAVE", "MOD OCTAVE MOD", "MOD OCTAVE SRC"},
	{MOD_INT_, "MOD INTERVAL", "MOD INTERVAL MOD", "MOD INTERVAL SRC"},
	{MOD_BEAT_, "MOD BEAT", "MOD BEAT MOD", "MOD BEAT SRC"},
	{GAIN_, "GAIN", "GAIN MOD", "GAIN SRC"},
	{FILTER_, "CUTOFF", "CUTOFF MOD", "CUTOFF SRC"},
	{PAN_LR, "PAN", "PAN MOD", "PAN SRC"},
};

const Group* groupOf (int pid, int& role)
{
	for (const Group& g : kGroups) {
		if (pid >= g.base && pid <= g.base + 2) {
			role = pid - g.base;
			return &g;
		}
	}
	return nullptr;
}

// Envelope and LFO blocks: ENV_n_* at 110 + 5(n-1), LFO_n_* at 125 + 5(n-1), LFO_n_PHASE at 140+.
const char* const kEnvNames[3][5] = {
	{"AMP ATTACK", "AMP DECAY", "AMP RELEASE", "AMP DECAY SWITCH", "AMP DELAY"},
	{"ENV 2 ATTACK", "ENV 2 DECAY", "ENV 2 RELEASE", "ENV 2 DECAY SWITCH", "ENV 2 DELAY"},
	{"ENV 3 ATTACK", "ENV 3 DECAY", "ENV 3 RELEASE", "ENV 3 DECAY SWITCH", "ENV 3 DELAY"},
};
const char* const kLfoNames[3][5] = {
	{"LFO 1 RATE", "LFO 1 DEPTH", "LFO 1 WAVE", "LFO 1 SYNC", "LFO 1 FADE IN"},
	{"LFO 2 RATE", "LFO 2 DEPTH", "LFO 2 WAVE", "LFO 2 SYNC", "LFO 2 FADE IN"},
	{"LFO 3 RATE", "LFO 3 DEPTH", "LFO 3 WAVE", "LFO 3 SYNC", "LFO 3 FADE IN"},
};
const char* const kLfoPhaseNames[3] = {"LFO 1 PHASE", "LFO 2 PHASE", "LFO 3 PHASE"};

const char* const kSources[7] = {"OFF", "AMP ENV", "ENV 2", "ENV 3", "LFO 1", "LFO 2", "LFO 3"};

} // namespace

std::string masterVolText (double v)
{
	const double g = masterVolGain (v);
	if (g < 1e-5)  // below -100 dB
		return "-inf dB";
	const double db = 20.0 * std::log10 (g);
	if (std::fabs (db) < 0.05)
		return "0.0 dB";
	return fmt ("%+.1f dB", db);
}

void voiceRange (const unsigned char* states, int lastLo, int lastHi, int& lo, int& hi)
{
	int mn = 128, mx = -1;
	for (int p = 0; p < 128; p++) {
		if (states[p]) {
			if (p < mn) mn = p;
			mx = p;
		}
	}
	if (mx < 0) {
		mn = lastLo;
		mx = lastHi;
	}
	if (mn < 0) mn = 0;
	if (mx > 127) mx = 127;
	if (mx < mn) mx = mn;
	const int kMinSpan = 12;
	if (mx - mn + 1 < kMinSpan) {
		mn -= (kMinSpan - (mx - mn + 1)) / 2;
		if (mn < 0) mn = 0;
		if (mn + kMinSpan - 1 > 127) mn = 128 - kMinSpan;
		mx = mn + kMinSpan - 1;
	}
	lo = mn;
	hi = mx;
}

void countVoices (const unsigned char* states, int& total, int& held)
{
	total = held = 0;
	for (int p = 0; p < 128; p++) {
		if (!states[p])
			continue;
		total++;
		if (states[p] & 0x80)
			held++;
	}
}

std::string peakText (float peak)
{
	if (!(peak >= 1.0e-5f))  // below -100 dB (or NaN)
		return "-inf dB";
	const double db = 20.0 * std::log10 (double (peak));
	if (std::fabs (db) < 0.05)
		return "0.0 dB";
	return fmt ("%+.1f dB", db);
}

const char* sourceName (int src)
{
	return (src >= 0 && src < 7) ? kSources[src] : "";
}

int sourceParamFor (int pid)
{
	int role = 0;
	const Group* g = groupOf (pid, role);
	return (g && role == 1) ? pid + 1 : -1;
}

int syncParamFor (int pid)
{
	if (pid == LFO_1_RATE)
		return LFO_1_BPM;
	if (pid == LFO_2_RATE)
		return LFO_2_BPM;
	if (pid == LFO_3_RATE)
		return LFO_3_BPM;
	return -1;
}

const char* label (int pid)
{
	int role = 0;
	if (const Group* g = groupOf (pid, role))
		return role == 0 ? g->name : (role == 1 ? g->amt : g->src);
	switch (pid) {
		case OSC_: return "OSC WAVE";
		case MOD_OSC_: return "MOD WAVE";
		case MOD_TYPE_: return "MOD TYPE";
		case FILTER_Q: return "RESONANCE";
		case MOD_OSC_GAIN: return "MOD LEVEL";
		case SYNC_GEN_MOD_TUNE: return "LINK MOD TUNE";
		case STEP_CTRL_TARG: return "STEP TARGET";
		case MASTER_VOL: return "VOLUME";
		case POLY_COUNT: return "POLYPHONY";
		case SNAP_TIME: return "SNAP";
		case EXPR_AMOUNT: return "EXPRESSION";
		case EXPR_TARGET: return "EXPR TARGET";
		case EXPR_VEL: return "EXPR VELOCITY";
		// the small squares above WIDTH/TUNE/FINE: click to centre that knob
		case WIDTH_RESET: return "OSC WIDTH RESET";
		case TUNE_RESET: return "OSC TUNE RESET";
		case FINE_RESET: return "OSC FINE RESET";
		case MOD_WIDTH_RESET: return "MOD WIDTH RESET";
		case MOD_TUNE_RESET: return "MOD TUNE RESET";
		case MOD_FINE_RESET: return "MOD FINE RESET";
		default: break;
	}
	if (pid >= ENV_1_ATTACK && pid < ENV_1_ATTACK + 15)
		return kEnvNames[(pid - ENV_1_ATTACK) / 5][(pid - ENV_1_ATTACK) % 5];
	if (pid >= LFO_1_RATE && pid < LFO_1_RATE + 15)
		return kLfoNames[(pid - LFO_1_RATE) / 5][(pid - LFO_1_RATE) % 5];
	if (pid >= LFO_1_PHASE && pid <= LFO_3_PHASE)
		return kLfoPhaseNames[pid - LFO_1_PHASE];
	return "";
}

double filterCutoffHz (double v)
{
	// Same curve as DXGenerator's filterLogScale2 (Steinberg LogScale: x^expo * range + min).
	const double lo = 80.0, hi = 18000.0, mid = 1800.0;
	const double expo = std::log ((mid - lo) / (hi - lo)) / std::log (0.5);
	return std::pow (clamp01 (v), expo) * (hi - lo) + lo;
}

double envTimeSeconds (int pid, double v)
{
	// One-pole coefficients exp(-exp(k - k*v) / fs) (DX10Processor::recalcEnv*): time constant
	// exp(k*v - k) seconds; 95% of the way after 3 time constants.
	v = clamp01 (v);
	const int which = (pid - ENV_1_ATTACK) % 5;
	double k = 8.0;          // attack
	if (which == 1) {        // decay
		if (v > 0.991)
			return -1.0;     // cdec = 1: no decay
		k = 6.0;
	}
	else if (which == 2) {   // release
		k = 5.0;
	}
	return 3.0 * std::exp (k * v - k);
}

double lfoRateHz (double v)
{
	return 0.08 + (20.0 - 0.08) * clamp01 (v);  // cookVSTGUIVariable(MIN_LFO_RATE, MAX_LFO_RATE)
}

const char* lfoSyncDivision (double v)
{
	// quants[] = 240,180,120,90,60,30,20,15,10,7.5,3.75 (rate = bpm / quants Hz, 60 = one beat).
	static const char* const kDiv[11] = {"1/1", "1/2.", "1/2", "1/4.", "1/4", "1/8",
	                                     "1/8T", "1/16", "1/16T", "1/32", "1/64"};
	int i = static_cast<int> (clamp01 (v) * 10);
	return kDiv[i < 0 ? 0 : (i > 10 ? 10 : i)];
}

std::string value (int pid, double v, const Context& ctx)
{
	int role = 0;
	if (const Group* g = groupOf (pid, role)) {
		if (role == 2)
			return sourceName (ctx.step);
		if (role == 1) {
			// Env amount, bipolar; the engine multiplies it by the source (0..1 or -1..1).
			const double amt = (v - 0.5) * 2.0;
			std::string s = fmt ("%+.0f %%", amt * 100.0);
			// Stepped targets: the engine truncates amount * range at full source level.
			int range = 0;
			const char* unit = "";
			if (g->base == INT_ || g->base == MOD_INT_) { range = 13; unit = " st"; }
			else if (g->base == OCT_) { range = 6; unit = " oct"; }
			else if (g->base == MOD_OCT_) { range = 13; unit = " oct"; }
			else if (g->base == MOD_BEAT_) { range = 13; unit = "/4 beat"; }
			if (range) {
				char buf[32];
				std::snprintf (buf, sizeof (buf), " (%+d%s)", static_cast<int> (amt * range), unit);
				s += buf;
			}
			if (ctx.srcStep >= 0)
				s += ctx.srcStep == 0 ? "  NO SRC" : std::string ("  < ") + sourceName (ctx.srcStep);
			return s;
		}
		switch (g->base) {
			case WIDTH_:
			case MOD_WIDTH_:
			case MOD_GAIN_:
				return percent (v);
			case GAIN_: {
				if (v <= 0.0)
					return "0 %  -inf dB";
				return fmt ("%.0f %%  %.1f dB", v * 100.0, 20.0 * std::log10 (v));
			}
			case TUNE_:
			case MOD_TUNE_:
				// Pitch = f * (1 + 2 * (tune - 0.5)) = f * 2 * tune (DXGenerator: tuneC).
				return ratioText (2.0 * v);
			case FINE_:
			case MOD_FINE_: {
				// Adds (fine - 0.5) / 100 to tuneC: ratio 1 + 0.02 * (fine - 0.5) at centre tune.
				const double ratio = 1.0 + 0.02 * (v - 0.5);
				return fmt ("%+.1f ct", 1200.0 * std::log2 (ratio));
			}
			case OCT_: {
				int o = ctx.step < 1 ? 1 : (ctx.step > 5 ? 5 : ctx.step);  // engine clamps 1..5
				return fmt ("%+.0f oct", static_cast<double> (o - 3));
			}
			case MOD_OCT_:
				return fmt ("%+.0f oct", static_cast<double> (ctx.step - 6));
			case INT_:
			case MOD_INT_:
				return fmt ("%+.0f st", static_cast<double> (ctx.step));
			case MOD_BEAT_:
				// Detune mode adds (bpm/60) * step/4 Hz to the modulator.
				return ctx.step == 0 ? "0" : fmt ("%.0f/4 per beat", static_cast<double> (ctx.step));
			case FILTER_:
				return hzText (filterCutoffHz (v));
			case PAN_LR: {
				const double p = (v - 0.5) * 2.0;
				if (std::fabs (p) < 0.005)
					return "CENTER";
				return fmt (p < 0 ? "L %.0f" : "R %.0f", std::fabs (p) * 100.0);
			}
			default:
				return percent (v);
		}
	}

	switch (pid) {
		case FILTER_Q:
			// cookVSTGUIVariable(1, 2.5) then CSEMFilter::setQControl (1..10 -> Q 0.5..25).
			return fmt ("Q %.2f", 0.5 + 24.5 * (1.5 * clamp01 (v)) / 9.0);
		case MOD_OSC_GAIN:
			return percent (v);
		case MASTER_VOL:
			return masterVolText (v);
		default:
			break;
	}

	if (pid >= ENV_1_ATTACK && pid < ENV_1_ATTACK + 15) {
		const int which = (pid - ENV_1_ATTACK) % 5;
		if (which <= 2)
			return timeText (envTimeSeconds (pid, v));
		return "";
	}
	if (pid >= LFO_1_RATE && pid < LFO_1_RATE + 15) {
		const int which = (pid - LFO_1_RATE) % 5;
		if (which == 0)
			return ctx.lfoSynced ? std::string (lfoSyncDivision (v)) : fmt ("%.2f Hz", lfoRateHz (v));
		if (which == 1)
			return percent (v);
		if (which == 4) {
			// The LFO waits this long at its start value, then runs (lfoShape.h; off at 0.03 and below).
			if (lfoShape::delaySeconds (v) <= 0.0)
				return "OFF";
			return timeText (clamp01 (v));
		}
		return "";
	}
	if (pid == SNAP_TIME)
		return fmt ("%.1f MS", snapMs (v));
	if (pid == EXPR_TARGET)
		return exprTargetName (exprTargetFromNormalized (v));
	if (pid == EXPR_AMOUNT) {
		const double a = (2.0 * v - 1.0) * 100.0;
		std::string s = std::fabs (a) < 0.5 ? std::string ("OFF") : fmt ("%+.0f %%", a);
		if (ctx.exprTarget >= 0 && std::fabs (a) >= 0.5)
			s += std::string (" ") + exprTargetName (ctx.exprTarget);
		if (ctx.exprVel && std::fabs (a) >= 0.5)
			s += " x VEL";
		return s;
	}
	if (pid == POLY_COUNT) {
		const int n = polyFromNormalized (v);
		return fmt (n == 1 ? "%.0f VOICE" : "%.0f VOICES", static_cast<double> (n));
	}
	if (pid >= LFO_1_PHASE && pid <= LFO_3_PHASE)
		{
			// Where in its cycle the LFO starts at note-on (lfoShape::phaseStart), 0..359 deg.
			if (ctx.lfoType < 0)
				return fmt ("%.2f", v);
			double s = lfoShape::phaseStart (ctx.lfoType, v);
			s -= std::floor (s);
			int deg = static_cast<int> (s * 360.0 + 0.5) % 360;
			return fmt ("START %.0f deg", static_cast<double> (deg));
		}
	return "";
}

} // namespace paramDisplay
