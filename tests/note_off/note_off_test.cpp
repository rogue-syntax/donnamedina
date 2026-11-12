// Note-off / retrigger regression test for Donna Medina (B-18, B-05).
//
// Drives the real DX10Processor (linked from the plugin's Release object files) through
// process() with VST3 event lists, exactly like a host would, and measures the output level
// after the notes should have ended. No host, no GUI, no controller.
//
// Build + run: tests\note_off\build_and_run.bat (after building Release|x64 of the plugin).

#include <windows.h>  // mdaBaseProcessor.h includes the Windows <bcrypt.h>, which needs this first
#include "../../source/mdaDX10Processor.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

// Normally defined by the SDK's dllmain.cpp, which the test does not link (it is an exe).
void* moduleHandle = nullptr;

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {

constexpr double kSampleRate = 44100.0;
constexpr int32 kBlock = 512;
constexpr int16 kPitch = 60;

struct NoteEvent {
	int64 sample;  // absolute sample position
	bool on;
	int16 pitch;
};

struct Settings {
	const char* name;
	double decay;        // ENV_1_DECAY normalised; > 0.991 = hold (no decay)
	double release;      // ENV_1_RELEASE normalised
};

// Renders `seconds` of audio for the given events and returns the peak level in each
// 10 ms window.
std::vector<float> render(const Settings& s, const std::vector<NoteEvent>& events, double seconds) {
	auto* proc = new rs::DX10Processor();
	proc->initialize(nullptr);
	ProcessSetup setup{kRealtime, kSample32, kBlock, kSampleRate};
	proc->setupProcessing(setup);
	proc->setActive(true);
	proc->regKeyLegit = true;  // bypass the registration gate for the test

	std::vector<float> left(kBlock), right(kBlock);
	float* chans[2] = {left.data(), right.data()};
	AudioBusBuffers out{};
	out.numChannels = 2;
	out.channelBuffers32 = chans;

	ProcessContext ctx{};
	ctx.sampleRate = kSampleRate;
	ctx.tempo = 120.0;
	ctx.state = ProcessContext::kTempoValid;

	const int64 total = static_cast<int64>(seconds * kSampleRate);
	const int64 window = static_cast<int64>(0.010 * kSampleRate);
	std::vector<float> peaks(static_cast<size_t>(total / window) + 1, 0.0f);

	int64 pos = 0;
	bool first = true;
	while (pos < total) {
		EventList evList;
		for (const auto& e : events) {
			if (e.sample >= pos && e.sample < pos + kBlock) {
				Event ev{};
				ev.busIndex = 0;
				ev.sampleOffset = static_cast<int32>(e.sample - pos);
				if (e.on) {
					ev.type = Event::kNoteOnEvent;
					ev.noteOn.channel = 0;
					ev.noteOn.pitch = e.pitch;
					ev.noteOn.velocity = 1.0f;
					ev.noteOn.noteId = -1;
				} else {
					ev.type = Event::kNoteOffEvent;
					ev.noteOff.channel = 0;
					ev.noteOff.pitch = e.pitch;
					ev.noteOff.velocity = 0.0f;
					ev.noteOff.noteId = -1;
				}
				evList.addEvent(ev);
			}
		}

		ParameterChanges changes;
		if (first) {  // envelope 1 = amp envelope: fast attack, decay/release from settings,
			          // decay toggle off (= use the release rate on note-off)
			auto set = [&](ParamID id, double v) {
				int32 idx = 0;
				if (auto* q = changes.addParameterData(id, idx)) {
					int32 pidx = 0;
					q->addPoint(0, v, pidx);
				}
			};
			set(ENV_1_ATTACK, 0.0);
			set(ENV_1_DECAY, s.decay);
			set(ENV_1_RELEASE, s.release);
			set(ENV_1_DECAYTOGGLE, 0.0);
			first = false;
		}

		ProcessData data{};
		data.processMode = kRealtime;
		data.symbolicSampleSize = kSample32;
		data.numSamples = kBlock;
		data.numOutputs = 1;
		data.outputs = &out;
		data.inputEvents = &evList;
		data.inputParameterChanges = &changes;
		data.processContext = &ctx;

		// The plugin shifts the host's channel pointers when a block has several event
		// offsets (B-03, not fixed yet), so reset them every block like most hosts do.
		chans[0] = left.data();
		chans[1] = right.data();
		std::fill(left.begin(), left.end(), 0.0f);
		std::fill(right.begin(), right.end(), 0.0f);

		proc->process(data);

		for (int32 i = 0; i < kBlock && pos + i < total; i++) {
			const float v = std::max(std::fabs(left[i]), std::fabs(right[i]));
			auto& pk = peaks[static_cast<size_t>((pos + i) / window)];
			pk = std::max(pk, v);
		}
		pos += kBlock;
	}

	proc->setActive(false);
	proc->terminate();
	proc->release();
	return peaks;
}

int64 at(double sec) { return static_cast<int64>(sec * kSampleRate); }

float peakAround(const std::vector<float>& peaks, double sec) {
	size_t i = static_cast<size_t>(sec / 0.010);
	return i < peaks.size() ? peaks[i] : 0.0f;
}

float maxPeak(const std::vector<float>& peaks) {
	return *std::max_element(peaks.begin(), peaks.end());
}

struct Case {
	const char* name;
	std::vector<NoteEvent> events;
	double checkSec;      // time at which we judge the result
	bool expectSounding;  // true: the key is still held at checkSec
};

}  // namespace

int main() {
	const Settings settings[] = {
		{"decay=HOLD", 1.0, 0.5},
		{"decay=0.6 ", 0.6, 0.5},
	};

	// Previous note on the same pitch is released at 0.30 s and is still audible at 0.35 s
	// (release tail), so the replay goes through the ~21 ms retrigger fade.
	std::vector<Case> cases = {
		{"single note, off @ 0.2 s",
		 {{0, true, kPitch}, {at(0.2), false, kPitch}}, 2.5, false},
		{"replay while tail sounds, off 10 ms later (inside fade)",
		 {{0, true, kPitch}, {at(0.30), false, kPitch}, {at(0.35), true, kPitch}, {at(0.36), false, kPitch}}, 2.5, false},
		{"replay while tail sounds, off 30 ms later (after fade)",
		 {{0, true, kPitch}, {at(0.30), false, kPitch}, {at(0.35), true, kPitch}, {at(0.38), false, kPitch}}, 2.5, false},
		{"back-to-back same pitch: off+on same sample, key held",
		 {{0, true, kPitch}, {at(0.30), false, kPitch}, {at(0.30), true, kPitch}}, 0.9, true},
		{"back-to-back same pitch: off+on same sample, then off @ 1.0 s",
		 {{0, true, kPitch}, {at(0.30), false, kPitch}, {at(0.30), true, kPitch}, {at(1.0), false, kPitch}}, 2.9, false},
	};
	{  // fast repeated notes: 1/32-ish at high tempo with a 15 ms gate
		Case arp{"fast repeats (40 ms period, 15 ms gate) x 25", {}, 3.0, false};
		for (int k = 0; k < 25; k++) {
			arp.events.push_back({at(0.04 * k), true, kPitch});
			arp.events.push_back({at(0.04 * k + 0.015), false, kPitch});
		}
		cases.push_back(arp);
	}

	int failures = 0;
	for (const auto& s : settings) {
		for (const auto& c : cases) {
			auto peaks = render(s, c.events, 3.1);
			const float ref = maxPeak(peaks);
			const float lvl = peakAround(peaks, c.checkSec);
			const float rel = ref > 0 ? lvl / ref : 0.0f;
			// "sounding" = above -40 dB of the peak; "ended" = below -60 dB.
			bool pass;
			if (c.expectSounding) {
				// With a finite decay a held note may legitimately have decayed away.
				pass = (s.decay < 0.991) ? true : rel > 0.01f;
			} else {
				pass = rel < 0.001f;
			}
			if (!pass) failures++;
			std::printf("%s  %-62s level @%.1fs = %7.4f of peak  expect %-9s %s\n", s.name, c.name,
			            c.checkSec, rel, c.expectSounding ? "sounding" : "ended", pass ? "PASS" : "FAIL");
		}
	}
	std::printf("\n%d failure(s)\n", failures);
	return failures == 0 ? 0 : 1;
}
