#pragma once
// FX strip (2026-11-15): chorus and a tempo-synced ping-pong delay after the voices (and after the
// HQ decimator), before the master volume. DX10Processor::doProcessing runs them per sample; buffers
// are allocated in prepare() (DX10Processor::setActive), never on the audio thread. Both always run
// (so turning one up does not replay an old buffer); with a mix of 0 they add exactly 0, so the
// output is unchanged (bit exact) with both off, as long as the lines only hold finite values.

#include <cmath>
#include <vector>
#include <algorithm>

namespace fx {

// Delay time: tempo divisions, shortest first (beats; 1 beat = a quarter note).
const int kDelayDivisions = 12;
inline double delayBeats(int i) {
	static const double k[kDelayDivisions] = {0.125, 1.0 / 6, 0.25, 1.0 / 3, 0.375, 0.5, 2.0 / 3, 0.75, 1.0, 1.5, 2.0, 4.0};
	return k[i < 0 ? 0 : (i >= kDelayDivisions ? kDelayDivisions - 1 : i)];
}
inline const char* delayDivisionName(int i) {
	static const char* const k[kDelayDivisions] = {"1/32", "1/16T", "1/16", "1/8T", "1/16.", "1/8",
	                                               "1/4T", "1/8.", "1/4", "1/4.", "1/2", "1/1"};
	return k[i < 0 ? 0 : (i >= kDelayDivisions ? kDelayDivisions - 1 : i)];
}
inline int delayDivisionFromNormalized(double v) {
	int i = static_cast<int>(v * (kDelayDivisions - 1) + 0.5);
	return i < 0 ? 0 : (i >= kDelayDivisions ? kDelayDivisions - 1 : i);
}
// Tone: one-pole low-pass in the feedback path, 800 Hz .. 16 kHz (log).
inline double delayToneHz(double v) { return 800.0 * std::pow(20.0, v < 0 ? 0 : (v > 1 ? 1 : v)); }
inline double delayFeedback(double v) { return 0.95 * (v < 0 ? 0 : (v > 1 ? 1 : v)); }
// Chorus rate 0.1 .. 5 Hz (log), depth 0 .. 5 ms of sweep around a 12 ms delay.
inline double chorusRateHz(double v) { return 0.1 * std::pow(50.0, v < 0 ? 0 : (v > 1 ? 1 : v)); }
inline double chorusDepthMs(double v) { return 5.0 * (v < 0 ? 0 : (v > 1 ? 1 : v)); }

// A delay line with linear-interpolated reads.
struct Line {
	std::vector<float> buf;
	int w = 0;
	void prepare(int n) { buf.assign(n > 4 ? n : 4, 0.0f); w = 0; }
	void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
	void write(float x) { buf[w] = x; if (++w >= int(buf.size())) w = 0; }
	// d samples behind the last write (1 = the last sample written)
	// B-67: the position used to be computed in float (w - d, then + n while negative). When w - d
	// was a tiny negative number, w - d + n rounded to exactly n (float spacing at n ~ 350000 is
	// 1/32), so it read buf[n], one past the end: whatever lay on the heap there went into the
	// output and the feedback loop. Now the whole part of d is an integer index, wrapped in int.
	float read(float d) const {
		const int n = int(buf.size());
		if (!(d >= 1.0f)) d = 1.0f;  // (also NaN)
		if (d > n - 2) d = float(n - 2);
		const int di = int(d);
		const float f = d - float(di);
		int i0 = w - di;              // di samples behind
		if (i0 < 0) i0 += n;
		const int i1 = i0 > 0 ? i0 - 1 : n - 1;  // one more behind
		const float a = buf[i0], b = buf[i1];
		return a + (b - a) * f;
	}
};

struct PingPongDelay {
	Line l, r;
	double sr = 44100.0;
	float time = -1.0f;  // smoothed delay in samples
	float lpL = 0.0f, lpR = 0.0f;
	void prepare(double sampleRate, double maxSeconds) {
		sr = sampleRate;
		l.prepare(int(sampleRate * maxSeconds) + 4);
		r.prepare(int(sampleRate * maxSeconds) + 4);
		reset();
	}
	void reset() { l.reset(); r.reset(); time = -1.0f; lpL = lpR = 0.0f; }
	// The mono sum goes into the left line; each line feeds the other, so echoes alternate L, R, L...
	void process(float& outL, float& outR, float targetSamples, float fb, float toneCoef, float mix) {
		if (time < 0.0f) time = targetSamples;
		time += (targetSamples - time) * 0.0005f;  // glide to a new time (tempo/division change) instead of jumping
		const float dl = l.read(time), dr = r.read(time);
		lpL += toneCoef * (dl - lpL);
		lpR += toneCoef * (dr - lpR);
		const float in = 0.5f * (outL + outR);
		l.write(in + fb * lpR);
		r.write(fb * lpL);
		outL += mix * dl;
		outR += mix * dr;
	}
};

struct Chorus {
	Line l, r;
	double sr = 44100.0, phase = 0.0;
	void prepare(double sampleRate) {
		sr = sampleRate;
		l.prepare(int(sampleRate * 0.03) + 4);  // up to 17 ms used
		r.prepare(int(sampleRate * 0.03) + 4);
		reset();
	}
	void reset() { l.reset(); r.reset(); phase = 0.0; }
	// Each channel reads its own line, swept by a sine 90 degrees apart between L and R.
	void process(float& outL, float& outR, double rateHz, double depthMs, float mix) {
		phase += rateHz / sr;
		if (phase >= 1.0) phase -= 1.0;
		const double tw = 6.283185307179586;
		const float base = float(0.012 * sr), sweep = float(depthMs * 0.001 * sr);
		const float dL = base + sweep * float(std::sin(tw * phase));
		const float dR = base + sweep * float(std::sin(tw * phase + tw * 0.25));
		l.write(outL);
		r.write(outR);
		const float wl = l.read(dL), wr = r.read(dR);
		const float g = 1.0f / (1.0f + 0.5f * mix);  // keep the level roughly constant
		outL = (outL + mix * wl) * g;
		outR = (outR + mix * wr) * g;
	}
};

} // namespace fx
