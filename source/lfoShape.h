#pragma once
// What an LFO does after note-on, in one place for the engine (DX10Processor recalcLfo*) and the
// LFO displays (NVGView::drawLfo), so the picture is the sound. No SDK dependency.
//
// Engine behaviour (CLFO / CLFOscillator, checked with `note_off_test lfo`):
// - Every note restarts the voice's LFO at its start phase (CLFOscillator::reset, moduloStart).
// - Delay: for `delaySeconds` the LFO stands still at its start value (update() sets m_dFo = 0 while
//   m_dDelayCounter < 1), then it runs. It is not a fade-in. Off below 0.03.
// - Output = wave(phase) * amp, bipolar -amp..+amp.

#include <cmath>
#include <string>

namespace lfoShape {

enum Wave { kSine = 0, kSaw = 1, kTri = 2, kSquare = 3 };

// LFO_n_TYPE (normalized, a 4-entry list: 0, 1/3, 2/3, 1) -> CLFO waveform. The old mapping
// div(v * 100, 25) gave 4 for the last entry, which CLFO::doOscillate has no case for: the square
// LFO was silent.
inline int typeFromNormalized(double v) {
	int t = static_cast<int>(v * 100.0) / 25;
	return t < 0 ? 0 : (t > 3 ? 3 : t);
}

// LFO_n_PHASE (normalized) -> start position in the cycle (CLFOscillator moduloStart). Sine: two
// cycles across the slider, (v - 0.5) * 2; square: v; saw and triangle: 1 - v.
inline double phaseStart(int type, double v) {
	if (type == kSine)
		return (v - 0.5) * 2.0;
	if (type == kSquare)
		return v;
	return 1.0 - v;
}

// LFO_n_DELAY (normalized) -> seconds the LFO waits at its start value (0 = off).
inline double delaySeconds(double v) {
	if (v <= 0.03)
		return 0.0;
	return v > 1.0 ? 1.0 : v;
}

// The waveform at a cycle position (any real number), -1..+1, as CLFO computes it.
inline double wave(int type, double pos) {
	double m = pos - std::floor(pos);
	switch (type) {
	case kSaw:  // falling ramp: +1 at the start of the cycle, -1 at the end
		return 1.0 - 2.0 * m;
	case kTri:  // -1 at 0, +1 at 0.5
		return m < 0.5 ? 4.0 * m - 1.0 : 3.0 - 4.0 * m;
	case kSquare:
		return m > 0.5 ? -1.0 : 1.0;
	default:  // CLFO::doSine: parabolicSine(pi - 2 pi m) = sin(2 pi m)
		return std::sin(6.283185307179586 * m);
	}
}

// LFO value t seconds after note-on (before amp), for rate hz.
inline double valueAt(int type, double start, double delay, double hz, double t) {
	if (t < delay)
		return wave(type, start);
	return wave(type, start + (t - delay) * hz);
}

// Display time window: the smallest of `ladder` (ascending) that holds the delay and two cycles.
inline double chooseWindow(double delay, double period, const double* ladder, int n) {
	const double need = delay + 2.0 * period;
	for (int i = 0; i < n; i++)
		if (ladder[i] >= need - 1e-9)
			return ladder[i];
	return ladder[n - 1];
}

// The window an LFO display shows. While the rate/delay/sync control is held (`editing`) it stays
// where it was, so turning the knob packs or stretches the wave; once released (or when the fitted
// window changes otherwise, e.g. host automation) it crossfades to the fitted one over kFade seconds.
struct ViewWindow {
	static constexpr double kFade = 0.3;
	double window = -1.0, fromWindow = -1.0;
	std::string text, fromText;
	double fadeStart = 0.0;
	bool fading = false;

	// Call once per frame with the fitted window and its label; returns the new view's opacity
	// (1 = settled; below 1 the old view, fromWindow/fromText, is drawn with 1 - that).
	double update(double fit, const std::string& fitText, bool editing, double now) {
		if (window <= 0.0) {
			window = fit;
			text = fitText;
		}
		if (!editing && (fit != window || fitText != text)) {
			fromWindow = window;
			fromText = text;
			window = fit;
			text = fitText;
			fadeStart = now;
			fading = true;
		}
		if (!fading)
			return 1.0;
		const double f = (now - fadeStart) / kFade;
		if (f >= 1.0) {
			fading = false;
			return 1.0;
		}
		return f < 0.0 ? 0.0 : f;
	}
};

} // namespace lfoShape
