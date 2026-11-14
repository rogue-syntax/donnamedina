#pragma once
// Oscilloscope capture for the right column's SCOPE band (2026-11-14).
//
// The audio thread calls push() for every output sample. It waits for a rising zero crossing of the
// mono sum (with a small hysteresis, so noise around 0 does not trigger), then records kLength points
// and publishes them. Without a crossing for kMaxWait points (silence, DC, very low notes) it captures
// untriggered. At sample rates of 88.2 kHz and up it keeps every n-th sample (n = rate / 44.1 kHz,
// rounded), so a frame covers about 21..23 ms at any rate (23.2 ms at 44.1, 21.3 ms at 48 and 96).
//
// Hand-off between the audio thread (writer) and the main-thread timer (reader) is a lock-free triple
// buffer: the writer fills its back buffer and swaps it with the middle one; the reader swaps the
// middle one with its front buffer when a new frame is there. Neither side ever waits or allocates.
// No SDK dependency, so it can be tested on its own.

#include <atomic>
#include <cmath>

class ScopeCapture {
public:
	static const int kLength = 1024;   // points per frame (~23 ms at 44.1 kHz)
	static const int kMaxWait = 2048;  // points to wait for a trigger before capturing anyway

	// Main thread, before processing starts.
	void setSampleRate (double sr)
	{
		int d = static_cast<int> (sr / 44100.0 + 0.5);
		decimation = d < 1 ? 1 : d;
		reset ();
	}

	void reset ()
	{
		state = kArming;
		waited = 0;
		pos = 0;
		phase = 0;
	}

	// Audio thread. Mono sample (e.g. (L + R) / 2).
	void push (float x)
	{
		if (++phase < decimation)
			return;
		phase = 0;
		switch (state) {
			case kArming:  // wait until the signal is clearly below 0, then for the crossing
				if (x < -kHysteresis)
					state = kArmed;
				else if (++waited >= kMaxWait)
					startCapture ();
				break;
			case kArmed:
				if (x >= 0.0f)
					startCapture ();
				else if (++waited >= kMaxWait)
					startCapture ();
				break;
			case kCapturing:
				break;
		}
		if (state == kCapturing) {
			buffers[back][pos++] = x;
			if (pos >= kLength) {
				back = middle.exchange (back | kFresh) & kIndexMask;
				reset ();
			}
		}
	}

	// Reader (main thread): if a new frame was published since the last call, returns it (kLength
	// points, valid until the next call), else nullptr.
	const float* latest ()
	{
		if ((middle.load () & kFresh) == 0)
			return nullptr;
		front = middle.exchange (front) & kIndexMask;
		return buffers[front];
	}

private:
	static constexpr float kHysteresis = 1.0e-4f;
	static const int kFresh = 4;
	static const int kIndexMask = 3;
	enum State { kArming, kArmed, kCapturing };

	void startCapture ()
	{
		state = kCapturing;
		pos = 0;
	}

	float buffers[3][kLength] = {};
	int back = 0;                    // writer
	std::atomic<int> middle {1};     // shared: index | kFresh
	int front = 2;                   // reader
	State state = kArming;
	int waited = 0;
	int pos = 0;
	int phase = 0;
	int decimation = 1;
};
