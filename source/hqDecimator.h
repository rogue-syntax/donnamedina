#pragma once
// HQ mode (2026-11-15): the voices run at twice the host rate; this halves it again. A 63-tap
// windowed-sinc low-pass (Blackman, cutoff 0.23 of the doubled rate = 0.46 of the host rate) removes
// what lies above the host's Nyquist before every second sample is dropped. Latency: 31 samples at
// the doubled rate (15.5 at the host rate). No allocation; one instance per channel.

#include <cmath>

struct HqDecimator {
	static const int kTaps = 63;
	float h[kTaps];
	float buf[2 * kTaps];  // each sample is written twice, so the newest kTaps are contiguous
	int pos = 0;

	HqDecimator() {
		const double fc = 0.23, pi = 3.14159265358979323846;
		const int m = kTaps - 1;
		double sum = 0.0;
		double t[kTaps];
		for (int n = 0; n < kTaps; n++) {
			const double x = n - m / 2.0;
			const double sinc = x == 0.0 ? 2.0 * fc : std::sin(2.0 * pi * fc * x) / (pi * x);
			const double w = 0.42 - 0.5 * std::cos(2.0 * pi * n / m) + 0.08 * std::cos(4.0 * pi * n / m);
			t[n] = sinc * w;
			sum += t[n];
		}
		for (int n = 0; n < kTaps; n++)
			h[n] = static_cast<float>(t[n] / sum);  // unity gain at DC
		reset();
	}

	void reset() {
		for (float& v : buf) v = 0.0f;
		pos = 0;
	}

	// Two consecutive samples at the doubled rate in, one sample at the host rate out.
	float process(float a, float b) {
		push(a);
		push(b);
		const float* x = buf + pos;  // x[0] oldest .. x[kTaps - 1] newest
		float y = 0.0f;
		for (int k = 0; k < kTaps; k++)
			y += h[k] * x[k];
		return y;
	}

private:
	void push(float v) {
		buf[pos] = v;
		buf[pos + kTaps] = v;
		pos = (pos + 1) % kTaps;
	}
};
