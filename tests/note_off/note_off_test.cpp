// Note-off / retrigger / output-buffer regression test for Donna Medina (B-18, B-05, B-03).
//
// Drives the real DX10Processor (linked from the plugin's Release object files) through
// process() with VST3 event lists, exactly like a host would, and measures the output level
// after the notes should have ended. No host, no GUI, no controller.
//
// Build + run: tests\note_off\build_and_run.bat (after building Release|x64 of the plugin).

#include <windows.h>  // mdaBaseProcessor.h includes the Windows <bcrypt.h>, which needs this first
#include "../../source/mdaDX10Processor.h"
#include "../../source/mdaBaseController.h"  // kPresetParam
#include "../../source/mdaDX10Controller.h"  // B-01/B-02 controller test
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <functional>
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
#include <algorithm>

#include <dbghelp.h>
#include <new>
#include <cstdlib>
#include <cstring>

// Normally defined by the SDK's dllmain.cpp, which the test does not link (it is an exe).
void* moduleHandle = nullptr;

// ---- B-10: heap use inside process() -------------------------------------------------------
// Replaces global operator new/delete for the whole exe (plugin objects and SDK libs are linked
// in statically, so their allocations come through here). While g_inProcess is true every
// allocation and free is counted and its call stack recorded, so the report shows where on the
// audio thread the heap is used. (malloc called directly is not seen.)
namespace heapwatch {
bool g_inProcess = false;  // set by the renderer around proc->process()
thread_local bool inHook = false;
struct Site {
	unsigned long long key;
	void* frames[16];
	unsigned short n;
	int allocs, frees;
	size_t bytes;
};
Site sites[1024];
int nSites = 0;
int allocs = 0, frees = 0;

void record(size_t bytes, bool isFree) {
	if (!g_inProcess || inHook) return;
	inHook = true;
	void* fr[16];
	unsigned short n = CaptureStackBackTrace(2, 16, fr, nullptr);
	unsigned long long key = isFree ? 1 : 2;
	for (unsigned short i = 0; i < n; i++) key = key * 1000003ULL + reinterpret_cast<unsigned long long>(fr[i]);
	int s = 0;
	while (s < nSites && sites[s].key != key) s++;
	if (s == nSites && nSites < 1024) {
		sites[s].key = key;
		std::memcpy(sites[s].frames, fr, sizeof(void*) * n);
		sites[s].n = n;
		sites[s].allocs = sites[s].frees = 0;
		sites[s].bytes = 0;
		nSites++;
	}
	if (s < 1024) {
		if (isFree) sites[s].frees++; else { sites[s].allocs++; sites[s].bytes += bytes; }
	}
	if (isFree) frees++; else allocs++;
	inHook = false;
}
void reset() { nSites = 0; allocs = frees = 0; }

// Prints each recorded site: the first frame outside the C++ runtime/std plus 3 callers.
void report(const char* indent) {
	static bool symInit = false;
	HANDLE proc = GetCurrentProcess();
	if (!symInit) {
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
		SymInitialize(proc, nullptr, TRUE);
		symInit = true;
	}
	inHook = true;  // the symbol engine allocates
	for (int s = 0; s < nSites; s++) {
		std::printf("%s%-5s x%-5d", indent, sites[s].allocs ? "new" : "free",
		            sites[s].allocs ? sites[s].allocs : sites[s].frees);
		int shown = 0;
		bool started = false;
		for (unsigned short f = 0; f < sites[s].n && shown < 4; f++) {
			char buf[sizeof(SYMBOL_INFO) + 512];
			auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
			sym->SizeOfStruct = sizeof(SYMBOL_INFO);
			sym->MaxNameLen = 511;
			DWORD64 disp = 0;
			const char* name = "?";
			if (SymFromAddr(proc, reinterpret_cast<DWORD64>(sites[s].frames[f]), &disp, sym)) name = sym->Name;
			const bool runtime = std::strncmp(name, "std::", 5) == 0 || std::strstr(name, "operator new") ||
			                     std::strstr(name, "operator delete") || std::strstr(name, "heapwatch") ||
			                     std::strncmp(name, "_", 1) == 0;
			if (!started && runtime) continue;
			started = true;
			IMAGEHLP_LINE64 line{};
			line.SizeOfStruct = sizeof(line);
			DWORD ld = 0;
			const char* file = "";
			DWORD ln = 0;
			if (SymGetLineFromAddr64(proc, reinterpret_cast<DWORD64>(sites[s].frames[f]), &ld, &line)) {
				file = std::strrchr(line.FileName, '\\') ? std::strrchr(line.FileName, '\\') + 1 : line.FileName;
				ln = line.LineNumber;
			}
			std::printf("%s %s (%s:%lu)", shown ? " <-" : "", name, file, ln);
			shown++;
		}
		std::printf("\n");
	}
	inHook = false;
}
}  // namespace heapwatch

void* operator new(size_t n) {
	heapwatch::record(n, false);
	if (void* p = std::malloc(n ? n : 1)) return p;
	throw std::bad_alloc();
}
void* operator new[](size_t n) { return operator new(n); }
void* operator new(size_t n, const std::nothrow_t&) noexcept {
	heapwatch::record(n, false);
	return std::malloc(n ? n : 1);
}
void* operator new[](size_t n, const std::nothrow_t& t) noexcept { return operator new(n, t); }
void operator delete(void* p) noexcept { if (p) { heapwatch::record(0, true); std::free(p); } }
void operator delete[](void* p) noexcept { operator delete(p); }
void operator delete(void* p, size_t) noexcept { operator delete(p); }
void operator delete[](void* p, size_t) noexcept { operator delete(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { operator delete(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { operator delete(p); }

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {

constexpr double kSampleRate = 44100.0;
constexpr int32 kBlock = 512;
constexpr int16 kPitch = 60;

int g_pointerDrift = 0;  // blocks after which the host's channel pointers had moved (B-03)
int g_overrun = 0;       // samples written past the end of the host buffer (B-03)

// B-13: counts IMessages the processor sends from inside process() (= from the audio thread).
bool g_inProcess = false;
int g_audioThreadMsgs = 0;
int g_deliveredMsgs = 0;  // B-10: messages delivered outside process() (by the main-thread timer)
int g_msgsIn[256] = {}, g_msgsOut[256] = {};  // per message ID (first char)
int g_badIdMsgs = 0;  // messages whose ID is not a 1-char C string (B-27)
void resetMsgCounts() {
	g_audioThreadMsgs = g_deliveredMsgs = 0;
	std::memset(g_msgsIn, 0, sizeof(g_msgsIn));
	std::memset(g_msgsOut, 0, sizeof(g_msgsOut));
}
// Runs the Windows message loop (the plugin's message timer fires through it, like in a host).
void pumpMessages() {
	MSG m;
	while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&m);
		DispatchMessage(&m);
	}
}
class CountingPeer : public IConnectionPoint {
public:
	tresult PLUGIN_API connect(IConnectionPoint*) override { return kResultTrue; }
	tresult PLUGIN_API disconnect(IConnectionPoint*) override { return kResultTrue; }
	tresult PLUGIN_API notify(IMessage* msg) override {
		const char* id = msg ? msg->getMessageID() : nullptr;
		if (id && std::strcmp(id, "InstanceID") == 0) return kResultTrue;  // sent by connect()
		const unsigned char c = id ? static_cast<unsigned char>(id[0]) : 0;
		if (!id || std::strlen(id) != 1) g_badIdMsgs++;  // B-27: IDs must be 1-char C strings
		if (g_inProcess) { g_audioThreadMsgs++; g_msgsIn[c]++; }
		else { g_deliveredMsgs++; g_msgsOut[c]++; }
		return kResultTrue;
	}
	tresult PLUGIN_API queryInterface(const TUID, void** obj) override { *obj = nullptr; return kNoInterface; }
	uint32 PLUGIN_API addRef() override { return 1; }
	uint32 PLUGIN_API release() override { return 1; }
};

struct NoteEvent {
	int64 sample;  // absolute sample position
	bool on;
	int16 pitch;
	int32 overshoot = 0;  // > 0: send in the block containing `sample` with offset kBlock + overshoot
	float velocity = 1.0f;  // note-on velocity, normalised like a host sends it (v / 127)
};

struct Options {
	bool nullContext = false;      // ProcessData.processContext = nullptr (allowed by VST3)
	bool nullEventsIfEmpty = false; // inputEvents = nullptr for blocks without events
	double tempo = 120.0;
	double tempo2 = 0.0, tempo2At = 0.0;  // if tempo2 > 0: host tempo changes at tempo2At seconds
	std::vector<std::pair<double, std::pair<ParamID, double>>> params;  // {seconds, {id, normalized}}
	std::function<void(rs::DX10Processor&)> inspect;  // called after the last block
	std::function<void(rs::DX10Processor&, double)> perBlock;  // after each block, with end time (s)
};

struct Settings {
	const char* name;
	double decay;        // ENV_1_DECAY normalised; > 0.991 = hold (no decay)
	double release;      // ENV_1_RELEASE normalised
};

// Renders `seconds` of audio for the given events and returns the peak level in each
// 10 ms window.
std::vector<float> render(const Settings& s, const std::vector<NoteEvent>& events, double seconds,
                          const Options& opt = {}) {
	// A real host context (so allocateMessage works) and a peer that counts messages.
	static HostApplication host;
	static CountingPeer peer;
	auto* proc = new rs::DX10Processor();
	proc->initialize(&host);
	proc->connect(&peer);
	ProcessSetup setup{kRealtime, kSample32, kBlock, kSampleRate};
	proc->setupProcessing(setup);
	proc->setActive(true);
	proc->regKeyLegit = true;  // bypass the registration gate for the test
	pumpMessages();
	resetMsgCounts();  // count only messages caused by processing

	// B-03 check: each channel buffer has a guard zone after it; any write there is an overrun.
	constexpr int32 kGuard = 8192;  // large enough that the old drift cannot pass it
	constexpr float kGuardVal = 12345.0f;
	std::vector<float> left(kBlock + kGuard, kGuardVal), right(kBlock + kGuard, kGuardVal);
	float* chans[2] = {left.data(), right.data()};
	AudioBusBuffers out{};
	out.numChannels = 2;
	out.channelBuffers32 = chans;

	ProcessContext ctx{};
	ctx.sampleRate = kSampleRate;
	ctx.tempo = opt.tempo;
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
				ev.sampleOffset = e.overshoot > 0 ? kBlock + e.overshoot : static_cast<int32>(e.sample - pos);
				if (e.on) {
					ev.type = Event::kNoteOnEvent;
					ev.noteOn.channel = 0;
					ev.noteOn.pitch = e.pitch;
					ev.noteOn.velocity = e.velocity;
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
		for (const auto& p : opt.params) {
			const int64 s0 = static_cast<int64>(p.first * kSampleRate);
			if (s0 >= pos && s0 < pos + kBlock) {
				int32 idx = 0;
				if (auto* q = changes.addParameterData(p.second.first, idx)) {
					int32 pidx = 0;
					q->addPoint(static_cast<int32>(s0 - pos), p.second.second, pidx);
				}
			}
		}
		if (opt.tempo2 > 0.0 && pos >= static_cast<int64>(opt.tempo2At * kSampleRate))
			ctx.tempo = opt.tempo2;

		ProcessData data{};
		data.processMode = kRealtime;
		data.symbolicSampleSize = kSample32;
		data.numSamples = kBlock;
		data.numOutputs = 1;
		data.outputs = &out;
		data.inputEvents = (opt.nullEventsIfEmpty && evList.getEventCount() == 0) ? nullptr : &evList;
		data.inputParameterChanges = &changes;
		data.processContext = opt.nullContext ? nullptr : &ctx;

		// Pointers are NOT reset between blocks: the plugin must leave them alone (B-03).
		std::fill(left.begin(), left.begin() + kBlock, 0.0f);
		std::fill(right.begin(), right.begin() + kBlock, 0.0f);

		g_inProcess = true;
		heapwatch::g_inProcess = true;
		proc->process(data);
		heapwatch::g_inProcess = false;
		g_inProcess = false;
		if (opt.perBlock) opt.perBlock(*proc, static_cast<double>(pos + kBlock) / kSampleRate);
		pumpMessages();

		if (chans[0] != left.data() || chans[1] != right.data()) {
			g_pointerDrift++;
			chans[0] = left.data();  // repair so the rest of the render is still measurable
			chans[1] = right.data();
		}
		for (int32 i = kBlock; i < kBlock + kGuard; i++) {
			if (left[i] != kGuardVal || right[i] != kGuardVal) {
				g_overrun++;
				left[i] = right[i] = kGuardVal;
			}
		}

		for (int32 i = 0; i < kBlock && pos + i < total; i++) {
			const float v = std::max(std::fabs(left[i]), std::fabs(right[i]));
			auto& pk = peaks[static_cast<size_t>((pos + i) / window)];
			pk = std::max(pk, v);
		}
		pos += kBlock;
	}

	for (int k = 0; k < 4; k++) {  // let the message timer (15 ms) deliver what is queued
		Sleep(20);
		pumpMessages();
	}
	if (opt.inspect) opt.inspect(*proc);
	proc->setActive(false);
	proc->disconnect(&peer);
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
	Options opt = {};
	bool checkSteady = false;  // also require a steady level 0.1-0.9 s (no retriggers while held)
};

float minPeak(const std::vector<float>& peaks, double from, double to) {
	float m = 1e9f;
	for (size_t i = static_cast<size_t>(from / 0.010); i < static_cast<size_t>(to / 0.010) && i < peaks.size(); i++)
		m = std::min(m, peaks[i]);
	return m;
}

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
	{  // dense MIDI (B-03): a note every 2 ms on rotating pitches -> ~5 event offsets per block
		Case dense{"dense MIDI (note every 2 ms, 4 pitches, 5 ms gate)", {}, 3.0, false};
		for (int k = 0; k < 400; k++) {
			const int16 p = static_cast<int16>(60 + (k % 4) * 3);
			dense.events.push_back({at(0.002 * k), true, p});
			dense.events.push_back({at(0.002 * k + 0.005), false, p});
		}
		cases.push_back(dense);
	}
	{  // control for the next case: same held note, event list present in every block
		Case c{"control: held note steady, off @ 1.0 s",
		       {{0, true, kPitch}, {at(1.0), false, kPitch}}, 2.9, false};
		c.checkSteady = true;
		cases.push_back(c);
	}
	{  // B-04: host sends no event list for blocks without events; key held 0..1.0 s
		Case c{"no event list on empty blocks: held note steady, off @ 1.0 s",
		       {{0, true, kPitch}, {at(1.0), false, kPitch}}, 2.9, false};
		c.opt.nullEventsIfEmpty = true;
		c.checkSteady = true;
		cases.push_back(c);
	}
	{  // B-04: note-off with an offset past the end of its block (out of spec)
		NoteEvent off{at(0.2), false, kPitch};
		off.overshoot = 10;
		cases.push_back({"note-off offset past block end", {{0, true, kPitch}, off}, 2.5, false});
	}
	{  // B-04: no ProcessContext at all (allowed by VST3). Old code dereferences it: crash. Keep last.
		Case c{"no ProcessContext", {{0, true, kPitch}, {at(0.2), false, kPitch}}, 2.5, false};
		c.opt.nullContext = true;
		cases.push_back(c);
	}

	int failures = 0;
	for (const auto& s : settings) {
		for (const auto& c : cases) {
			std::fflush(stdout);  // so output survives if a case crashes
			auto peaks = render(s, c.events, 3.1, c.opt);
			const float ref = maxPeak(peaks);
			const float lvl = peakAround(peaks, c.checkSec);
			const float rel = ref > 0 ? lvl / ref : 0.0f;
			if (c.checkSteady && s.decay >= 0.991) {
				const float steady = ref > 0 ? minPeak(peaks, 0.1, 0.9) / ref : 0.0f;
				// A normally held note of the default patch measures ~0.48 here (its level varies);
				// retriggering every block (old B-04) measured 0.0002.
				const bool ok = steady > 0.25f;
				if (!ok) failures++;
				std::printf("%s  %-62s min level 0.1-0.9s = %7.4f of peak  expect steady    %s\n", s.name, c.name,
				            steady, ok ? "PASS" : "FAIL");
			}
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
	// ---- B-13: tempo handling of the LFOs, and messages sent from the audio thread ----
	{
		std::printf("\n");
		const std::vector<NoteEvent> held = {{0, true, kPitch}, {at(2.0), false, kPitch}};
		const double rateNorm = 0.5;  // LFO rate knob; with sync on -> quants[5]
		struct TCase {
			const char* name;
			Options o;
			int maxMsgs;        // messages allowed from process() over the whole 3.1 s render
			double expectRate;  // expected final LFO 1 rate, < 0 = not checked
		};
		auto syncOn = [&](double at0, double atSync) {
			return std::vector<std::pair<double, std::pair<ParamID, double>>>{
				{at0, {LFO_1_RATE, rateNorm}}, {atSync, {LFO_1_BPM, 1.0}}};
		};
		std::vector<TCase> tcases;
		{ TCase t{"tempo 133.3, sync off (held note)", {}, 3, -1.0}; t.o.tempo = 133.3; tcases.push_back(t); }
		{ TCase t{"tempo 133.3, LFO 1 sync on", {}, 6, 133.3 / quants[5]}; t.o.tempo = 133.3;
		  t.o.params = syncOn(0.0, 0.0); tcases.push_back(t); }
		{ TCase t{"LFO 1 sync on, tempo 120 -> 140 at 1.0 s", {}, 8, 140.0 / quants[5]}; t.o.tempo = 120.0;
		  t.o.tempo2 = 140.0; t.o.tempo2At = 1.0; t.o.params = syncOn(0.0, 0.0); tcases.push_back(t); }
		{ TCase t{"LFO 1 sync switched on at 0.5 s, tempo 133.3", {}, 8, 133.3 / quants[5]}; t.o.tempo = 133.3;
		  t.o.params = syncOn(0.0, 0.5); tcases.push_back(t); }
		{ TCase t{"LFO 1 sync switched on at 0.5 s, tempo 120", {}, 8, 120.0 / quants[5]}; t.o.tempo = 120.0;
		  t.o.params = syncOn(0.0, 0.5); tcases.push_back(t); }

		for (auto& t : tcases) {
			double rate = 0.0;
			t.o.inspect = [&](rs::DX10Processor& p) { rate = p.EnvLfoCtrls[4].rate; };
			g_audioThreadMsgs = 0;
			std::fflush(stdout);
			render(settings[0], held, 3.1, t.o);
			// all messages caused by processing: sent inside process() (old) or queued and delivered
			// by the timer (since B-10)
			const int msgs = g_audioThreadMsgs + g_deliveredMsgs;
			bool ok = msgs <= t.maxMsgs;
			if (t.expectRate >= 0.0) ok = ok && std::fabs(rate - t.expectRate) <= 1e-3 * t.expectRate;
			if (!ok) failures++;
			std::printf("B-13  %-48s msgs = %4d (max %d)  LFO1 rate = %8.4f", t.name, msgs, t.maxMsgs, rate);
			if (t.expectRate >= 0.0) std::printf(" (expect %.4f)", t.expectRate);
			std::printf("  %s\n", ok ? "PASS" : "FAIL");
		}
	}

	// ---- B-17: finished voices leave the render list ----
	{
		std::printf("\n");
		struct VCase {
			const char* name;
			std::vector<NoteEvent> events;
			Options o;
			double sizeAt;   // time at which the render-list size is checked
			int maxSize;     // allowed size at sizeAt
			int minSize;     // required size at sizeAt
			double soundAt;  // < 0: not checked; else must be sounding (> 1% of peak) here
			double silentAt; // < 0: not checked; else must have ended (< -60 dB) here
			double seconds = 5.0;
		};
		std::vector<VCase> vcases;
		{  // 60 different pitches, 30 ms each, one after another; all released by 1.8 s
			VCase v{"60 distinct pitches, all released", {}, {}, 4.5, 0, 0, -1, 4.5};
			for (int k = 0; k < 60; k++) {
				v.events.push_back({at(0.03 * k), true, static_cast<int16>(30 + k)});
				v.events.push_back({at(0.03 * k + 0.02), false, static_cast<int16>(30 + k)});
			}
			vcases.push_back(v);
		}
		{  // one note held (decay = hold) while 20 other pitches come and go
			VCase v{"1 held note among 20 released pitches", {{0, true, 40}}, {}, 4.5, 1, 1, 4.5, -1};
			for (int k = 0; k < 20; k++) {
				v.events.push_back({at(0.05 * k + 0.1), true, static_cast<int16>(60 + k)});
				v.events.push_back({at(0.05 * k + 0.13), false, static_cast<int16>(60 + k)});
			}
			vcases.push_back(v);
		}
		{  // slow attack: the voice must not be dropped while cenv is still tiny
			VCase v{"slow attack (ENV_1_ATTACK 0.8), held", {{0, true, kPitch}}, {}, 0.02, 1, 1, 1.5, -1};
			v.o.params = {{0.0, {ENV_1_ATTACK, 0.8}}};
			vcases.push_back(v);
		}
		{  // replay a pitch after its voice was removed: fresh note, sounds, ends
			VCase v{"replay pitch after its voice was removed",
			        {{0, true, kPitch}, {at(0.1), false, kPitch}, {at(2.5), true, kPitch}, {at(2.6), false, kPitch}},
			        {}, 2.45, 0, 0, 2.55, 4.9};
			vcases.push_back(v);
		}
		{  // 40 held notes in one block: voice limit (POLYCOUNT = 32) must hold
			VCase v{"40 held notes (limit 32)", {}, {}, 1.0, 32, 32, 1.0, -1};
			for (int k = 0; k < 40; k++) v.events.push_back({0, true, static_cast<int16>(40 + k)});
			vcases.push_back(v);
		}

		for (auto& v : vcases) {
			int sizeSeen = -1;
			int maxSeen = 0;
			v.o.perBlock = [&](rs::DX10Processor& p, double t) {
				const int n = static_cast<int>(p.generators[0].notes.size());
				maxSeen = std::max(maxSeen, n);
				if (sizeSeen < 0 && t >= v.sizeAt) sizeSeen = n;
			};
			std::fflush(stdout);
			const auto t0 = GetTickCount64();
			auto peaks = render(settings[0], v.events, v.seconds, v.o);
			const auto ms = GetTickCount64() - t0;
			const float ref = maxPeak(peaks);
			bool ok = sizeSeen >= v.minSize && sizeSeen <= v.maxSize;
			float sLvl = -1, eLvl = -1;
			if (v.soundAt >= 0) { sLvl = ref > 0 ? peakAround(peaks, v.soundAt) / ref : 0; ok = ok && sLvl > 0.01f; }
			if (v.silentAt >= 0) { eLvl = ref > 0 ? peakAround(peaks, v.silentAt) / ref : 0; ok = ok && eLvl < 0.001f; }
			if (!ok) failures++;
			std::printf("B-17  %-40s voices @%.2fs = %3d (want %d..%d, max seen %3d)", v.name, v.sizeAt, sizeSeen,
			            v.minSize, v.maxSize, maxSeen);
			if (sLvl >= 0) std::printf("  sounding @%.2fs %.4f", v.soundAt, sLvl);
			if (eLvl >= 0) std::printf("  ended @%.2fs %.4f", v.silentAt, eLvl);
			std::printf("  [%llu ms]  %s\n", ms, ok ? "PASS" : "FAIL");
		}
	}

	// ---- B-10: heap allocations / frees inside process() (report only, not a pass/fail yet) ----
	{
		std::printf("\nB-10 heap use and messages inside process() (operator new/delete; host message objects included)\n");
		struct HCase {
			const char* name;
			std::vector<NoteEvent> events;
			Options o;
			double seconds;
		};
		std::vector<HCase> hcases;
		hcases.push_back({"idle (no notes)", {}, {}, 1.0});
		hcases.push_back({"one held note", {{0, true, kPitch}}, {}, 2.0});
		{
			HCase h{"playing: 4-note arp, 50 ms steps, 3 s", {}, {}, 3.0};
			for (int k = 0; k < 60; k++) {
				const int16 p = static_cast<int16>(60 + (k % 4) * 4);
				h.events.push_back({at(0.05 * k), true, p});
				h.events.push_back({at(0.05 * k + 0.03), false, p});
			}
			hcases.push_back(h);
		}
		{
			HCase h{"60 distinct pitches", {}, {}, 3.0};
			for (int k = 0; k < 60; k++) {
				h.events.push_back({at(0.03 * k), true, static_cast<int16>(30 + k)});
				h.events.push_back({at(0.03 * k + 0.02), false, static_cast<int16>(30 + k)});
			}
			hcases.push_back(h);
		}
		{  // every generator param (0..54) and env/LFO/step param (110..143), one per block, note held
			HCase h{"knob sweep: params 0-54 and 110-143", {{0, true, kPitch}}, {}, 2.5};
			double t = 0.05;
			for (int id = 0; id < PARTPARAMS; id++, t += 0.02) h.o.params.push_back({t, {static_cast<ParamID>(id), 0.3}});
			for (int id = ENV_1_ATTACK; id <= STEP_CTRL_TARG; id++, t += 0.02)
				h.o.params.push_back({t, {static_cast<ParamID>(id), 0.3}});
			hcases.push_back(h);
		}
		{
			HCase h{"preset changes (7 programs)", {{0, true, kPitch}}, {}, 2.0};
			for (int k = 0; k < 7; k++)
				h.o.params.push_back({0.1 + 0.2 * k, {rs::BaseController::kPresetParam, (k + 0.5) / 7.0}});
			hcases.push_back(h);
		}
		{
			HCase h{"LFO 1 synced, tempo 120 -> 140 at 1.0 s", {{0, true, kPitch}}, {}, 2.0};
			h.o.params = {{0.0, {LFO_1_RATE, 0.5}}, {0.0, {LFO_1_BPM, 1.0}}};
			h.o.tempo2 = 140.0;
			h.o.tempo2At = 1.0;
			hcases.push_back(h);
		}
		auto byId = [](const int* counts) {
			static char buf[512];
			buf[0] = 0;
			for (int c = 1; c < 256; c++)
				if (counts[c]) std::snprintf(buf + std::strlen(buf), sizeof(buf) - std::strlen(buf), " '%c':%d", c, counts[c]);
			return buf;
		};
		for (auto& h : hcases) {
			heapwatch::reset();
			std::fflush(stdout);
			render(settings[0], h.events, h.seconds, h.o);
			const bool ok = heapwatch::allocs == 0 && heapwatch::frees == 0 && g_audioThreadMsgs == 0;
			if (!ok) failures++;
			std::printf("  %-42s allocs %5d  frees %5d  msgs sent in process %4d, delivered later %4d  %s\n", h.name,
			            heapwatch::allocs, heapwatch::frees, g_audioThreadMsgs, g_deliveredMsgs, ok ? "PASS" : "FAIL");
			if (g_audioThreadMsgs) std::printf("      in process by id:%s\n", byId(g_msgsIn));
			if (g_deliveredMsgs) std::printf("      delivered by id:  %s\n", byId(g_msgsOut));
			if (!ok) heapwatch::report("      ");
		}
	}

	// ---- B-52: unknown parameter ids from a host must not crash or corrupt ----
	{
		std::printf("\nB-52 unknown parameter ids (60: generator 2 that does not exist; 150, 999: no recalculator)\n");
		std::fflush(stdout);
		Options o;
		o.params = {{0.1, {60, 0.7}}, {0.2, {150, 0.7}}, {0.3, {999, 0.7}}, {0.4, {99, 0.3}}};
		auto peaks = render(settings[0], {{0, true, kPitch}, {at(1.0), false, kPitch}}, 3.0, o);
		const float ref = maxPeak(peaks);
		const float held = ref > 0 ? peakAround(peaks, 0.8) / ref : 0.0f;
		const float ended = ref > 0 ? peakAround(peaks, 2.9) / ref : 1.0f;
		const bool ok = held > 0.25f && ended < 0.001f;
		if (!ok) failures++;
		std::printf("  survived; note still sounding @0.8s %.4f, ended @2.9s %.4f  %s\n", held, ended, ok ? "PASS" : "FAIL");
	}

	// ---- B-06: velocity -> level ----
	{
		std::printf("\nB-06 velocity -> level (held note, decay = hold, peak 0.3-0.6 s, relative to velocity 127)\n");
		const int vels[] = {1, 16, 32, 48, 64, 80, 96, 112, 127};
		float lv[9] = {};
		for (int k = 0; k < 9; k++) {
			NoteEvent on{0, true, kPitch};
			on.velocity = vels[k] / 127.0f;
			auto peaks = render(settings[0], {on}, 0.7);
			float pk = 0.0f;
			for (size_t i = 30; i < 60 && i < peaks.size(); i++) pk = std::max(pk, peaks[i]);
			lv[k] = pk;
		}
		int distinct = 1;
		bool monotonic = true;
		for (int k = 1; k < 9; k++) {
			if (std::fabs(lv[k] - lv[k - 1]) > 1e-4f * lv[8]) distinct++;
			if (lv[k] + 1e-6f < lv[k - 1]) monotonic = false;
		}
		bool linear = true;
		for (int k = 0; k < 9; k++) {
			const float rel = lv[8] > 0 ? lv[k] / lv[8] : 0.0f;
			const float want = vels[k] / 127.0f;
			std::printf("  vel %3d  level %.4f of vel 127  (linear would be %.4f)\n", vels[k], rel, want);
			if (std::fabs(rel - want) > 0.03f) linear = false;
		}
		const bool ok = monotonic && distinct == 9 && linear && lv[0] > 0.0f;
		if (!ok) failures++;
		std::printf("  %d of 9 velocities give distinct levels, monotonic %s, linear %s, vel 1 audible %s  %s\n", distinct,
		            monotonic ? "yes" : "no", linear ? "yes" : "no", lv[0] > 0.0f ? "yes" : "no", ok ? "PASS" : "FAIL");
	}

	// ---- B-01 / B-02: controller with the editor closed (no view, as when the GUI is not open) ----
	{
		std::printf("\nB-01/B-02 controller, editor closed\n");
		static HostApplication chost;
		static CountingPeer cpeer;
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-58s %-40s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[128];
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&chost);
		g_badIdMsgs = 0;
		ctrl->connect(&cpeer);
		pumpMessages();
		resetMsgCounts();
		const bool viewClosed = !ctrl->isViewReady();
		check("editor is closed (nvgViewReady false)", viewClosed, "");

		// B-02: host automation of a continuous param
		ctrl->setParamNormalized(FILTER_, 0.37);
		const double f = ctrl->getParamNormalized(FILTER_);
		std::snprintf(buf, sizeof(buf), "stored %.3f (sent 0.370)", f);
		check("automation of FILTER_ is stored", std::fabs(f - 0.37) < 1e-6, buf);

		// B-02: host automation of a vertical switch: the processor must get the integer position
		resetMsgCounts();
		ctrl->setParamNormalized(OCT_, 0.8);
		const double o = ctrl->getParamNormalized(OCT_);
		const int sw = g_msgsOut[static_cast<unsigned char>(PMSG_STEP_SWITCH_CHAR)];
		std::snprintf(buf, sizeof(buf), "stored %.3f, switch msgs to processor %d", o, sw);
		check("automation of OCT_ (vertical switch) stored + sent", std::fabs(o - 0.8) < 1e-6 && sw >= 1, buf);

		// B-01: preset change with the editor closed
		const int prog = 2;
		ctrl->setParamNormalized(rs::BaseController::kPresetParam, (prog + 0.5) / rs::DX10Processor::kNumPrograms);
		int mismatches = 0;
		const ParamID probe[] = {OSC_, WIDTH_, FILTER_, GAIN_, ENV_1_ATTACK, ENV_1_DECAY, LFO_1_RATE, MOD_OSC_};
		for (ParamID id : probe)
			if (std::fabs(ctrl->getParamNormalized(id) - rs::DX10Processor::programParams[prog][id]) > 1e-6) mismatches++;
		std::snprintf(buf, sizeof(buf), "%d of 8 probed params differ from preset %d", mismatches, prog);
		check("preset change applied to the controller", mismatches == 0, buf);

		// the GUI update is queued for when the editor opens
		std::snprintf(buf, sizeof(buf), "%zu queued", ctrl->pendingStateUpdates.size());
		check("GUI updates queued for onNVGViewReady", !ctrl->pendingStateUpdates.empty(), buf);

		std::snprintf(buf, sizeof(buf), "%d message(s) with a bad ID", g_badIdMsgs);
		check("B-27: controller message IDs are 1-char C strings", g_badIdMsgs == 0, buf);

		ctrl->disconnect(&cpeer);
		ctrl->terminate();
		ctrl->release();
	}

	// ---- B-21: TRI waveshape across the width range (oscillator alone) ----
	{
		std::printf("\nB-21 TRI waveshape, CQBLimitedOscillator alone, 44.1 kHz\n");
		const int N = 65536;
		auto fft = [](std::vector<std::complex<double>>& a) {  // iterative radix-2
			const size_t n = a.size();
			for (size_t i = 1, j = 0; i < n; i++) {
				size_t bit = n >> 1;
				for (; j & bit; bit >>= 1) j ^= bit;
				j ^= bit;
				if (i < j) std::swap(a[i], a[j]);
			}
			for (size_t len = 2; len <= n; len <<= 1) {
				const double ang = -2.0 * 3.14159265358979323846 / static_cast<double>(len);
				const std::complex<double> wl(std::cos(ang), std::sin(ang));
				for (size_t i = 0; i < n; i += len) {
					std::complex<double> w(1.0);
					for (size_t k = 0; k < len / 2; k++) {
						const auto u = a[i + k], v = a[i + k + len / 2] * w;
						a[i + k] = u + v;
						a[i + k + len / 2] = u - v;
						w *= wl;
					}
				}
			}
		};
		// f0 = fs * m / N: every harmonic sits exactly on a bin that is a multiple of m; aliased
		// (folded) harmonics land on other bins. alias = power off the harmonic bins / harmonic power.
		const int bins[] = {327, 2229};  // ~220 Hz and ~1500 Hz
		const double widths[] = {0.1, 0.3, 0.5, 0.7, 0.9};
		for (int m : bins) {
			for (double w : widths) {
				double fs = kSampleRate;
				CQBLimitedOscillator osc;
				osc.setSampleRate(&fs);
				osc.m_uWaveform = 2;  // TRI (doTriangle2)
				osc.triCutoff = w;
				osc.m_dOscFo = fs * m / N;
				osc.update();
				osc.startOscillator();
				osc.update();
				std::vector<std::complex<double>> x(N);
				double mn = 1e9, mx = -1e9, sum = 0.0;
				for (int i = 0; i < N; i++) {
					const double v = osc.doOscillate();
					x[i] = v;
					mn = std::min(mn, v);
					mx = std::max(mx, v);
					sum += v;
				}
				fft(x);
				double harm = 0.0, inh = 0.0;
				for (int b = 1; b < N / 2; b++) {
					const double p = std::norm(x[b]);
					if (b % m == 0) harm += p; else inh += p;
				}
				const double aliasDb = 10.0 * std::log10(std::max(inh, 1e-30) / std::max(harm, 1e-30));
				const bool bounded = mx <= 1.0 + 1e-9 && mn >= -1.0 - 1e-9;
				if (!bounded) failures++;
				std::printf("  f0 %6.1f Hz  width %.1f  min %6.3f  max %6.3f  DC %6.3f  alias %7.1f dB  %s\n",
				            fs * m / N, w, mn, mx, sum / N, aliasDb, bounded ? "PASS" : "FAIL (outside +-1)");
			}
		}
	}

	std::printf("\nB-03: host pointer moved after %d block(s), %d sample(s) written past buffer end  %s\n",
	            g_pointerDrift, g_overrun, (g_pointerDrift == 0 && g_overrun == 0) ? "PASS" : "FAIL");
	if (g_pointerDrift != 0 || g_overrun != 0) failures++;
	std::printf("\n%d failure(s)\n", failures);
	return failures == 0 ? 0 : 1;
}
