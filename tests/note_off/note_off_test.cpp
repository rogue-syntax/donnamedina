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
#include "../../source/paramDisplay.h"
#include "../../source/presetTools.h"
#include "../../source/lfoShape.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/common/memorystream.h"
#include "base/source/fstreamer.h"
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
double g_peakMsgMax = 0.0;  // largest "peak" in PMSG_PEAK_CHAR messages
std::vector<char> g_stateMsg;  // data of the last PMSG_STATE_CHAR message
int64 g_stateMsgPurpose = -1;
int g_peakMsgZeroRun = 0, g_peakMsgZeroRunMax = 0;  // consecutive PMSG_PEAK_CHAR messages with peak 0
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
		if (c == static_cast<unsigned char>(PMSG_STATE_CHAR)) {
			const void* d = nullptr;
			uint32 n = 0;
			if (msg->getAttributes()->getBinary("data", d, n) == kResultTrue && d)
				g_stateMsg.assign(static_cast<const char*>(d), static_cast<const char*>(d) + n);
			msg->getAttributes()->getInt("purpose", g_stateMsgPurpose);
		}
		if (c == static_cast<unsigned char>(PMSG_PEAK_CHAR)) {
			double pk = -1.0;
			msg->getAttributes()->getFloat("peak", pk);
			if (pk > g_peakMsgMax) g_peakMsgMax = pk;
			g_peakMsgZeroRun = pk == 0.0 ? g_peakMsgZeroRun + 1 : 0;
			if (g_peakMsgZeroRun > g_peakMsgZeroRunMax) g_peakMsgZeroRunMax = g_peakMsgZeroRun;
		}
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
	bool keepPatchEnvelopes = false;    // true: do not override ENV_1 attack/decay/release (default patch)
	IConnectionPoint* peer = nullptr;   // connect the processor to this instead of the counting peer
	std::function<void(rs::DX10Processor&)> onCreated;  // after initialize/connect, before processing
	std::vector<float>* captureL = nullptr;  // if set: receives every output sample (left)
	std::vector<float>* captureR = nullptr;
	double sampleRate = 0.0;  // 0 = kSampleRate; events (NoteEvent::sample) are in samples at this rate
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
	const double sr = opt.sampleRate > 0.0 ? opt.sampleRate : kSampleRate;  // HQ/B-63 tests run at other rates
	// A real host context (so allocateMessage works) and a peer that counts messages.
	static HostApplication host;
	static CountingPeer peer;
	auto* proc = new rs::DX10Processor();
	proc->initialize(&host);
	IConnectionPoint* usedPeer = opt.peer ? opt.peer : static_cast<IConnectionPoint*>(&peer);
	proc->connect(usedPeer);
	if (opt.onCreated) opt.onCreated(*proc);
	ProcessSetup setup{kRealtime, kSample32, kBlock, sr};
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
	ctx.sampleRate = sr;
	ctx.tempo = opt.tempo;
	ctx.state = ProcessContext::kTempoValid;

	const int64 total = static_cast<int64>(seconds * sr);
	const int64 window = static_cast<int64>(0.010 * sr);
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
		if (first && opt.keepPatchEnvelopes) first = false;
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
			const int64 s0 = static_cast<int64>(p.first * sr);
			if (s0 >= pos && s0 < pos + kBlock) {
				int32 idx = 0;
				if (auto* q = changes.addParameterData(p.second.first, idx)) {
					int32 pidx = 0;
					q->addPoint(static_cast<int32>(s0 - pos), p.second.second, pidx);
				}
			}
		}
		if (opt.tempo2 > 0.0 && pos >= static_cast<int64>(opt.tempo2At * sr))
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
		if (opt.perBlock) opt.perBlock(*proc, static_cast<double>(pos + kBlock) / sr);
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
			if (opt.captureL) opt.captureL->push_back(left[i]);
			if (opt.captureR) opt.captureR->push_back(right[i]);
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
	proc->disconnect(usedPeer);
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

// Probe (not a pass/fail test): default patch, filter turned down, the same note tapped
// rhythmically so each tap cuts into the previous note's release. Looks for notes whose restart
// differs from the rest (pops). Run: note_off_test.exe retrig [filter] [periodMs] [gateMs] [seconds]
int retrigProbe(int argc, char** argv) {
	const double filter = argc > 2 ? std::atof(argv[2]) : 0.3;
	const double period = (argc > 3 ? std::atof(argv[3]) : 300.0) / 1000.0;
	const double gate = (argc > 4 ? std::atof(argv[4]) : 120.0) / 1000.0;
	const double seconds = argc > 5 ? std::atof(argv[5]) : 30.0;
	const Settings s{"patch", 0.0, 0.0};
	std::vector<NoteEvent> ev;
	// RETRIG_JITTER=1: human-like timing: period +-25%, gate 40%..160%, velocity 70..127 (fixed seed)
	const bool jitter = std::getenv("RETRIG_JITTER") != nullptr;
	unsigned rng = 12345u;
	auto rnd = [&rng]() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 16777216.0; };
	for (double t = 0.05; t + 2 * gate < seconds - 1.0;) {
		const double g = jitter ? gate * (0.4 + 1.2 * rnd()) : gate;
		NoteEvent on{at(t), true, kPitch};
		if (jitter) on.velocity = static_cast<float>((70 + 57 * rnd()) / 127.0);
		ev.push_back(on);
		ev.push_back({at(t + g), false, kPitch});
		t += jitter ? period * (0.75 + 0.5 * rnd()) : period;
	}
	Options o;
	o.keepPatchEnvelopes = true;
	// re-select preset 0 at t=0 so the processor sends its switch values and the real controller
	// answers with the integer positions (osc type, mod type, env sources), as in a host
	o.params = {{0.0, {rs::BaseController::kPresetParam, 0.5 / rs::DX10Processor::kNumPrograms}},
	            {0.02, {FILTER_, filter}}};
	static HostApplication chost;
	auto* ctrl = new rs::DX10Controller();
	ctrl->initialize(&chost);
	const bool noCtrl = std::getenv("RETRIG_NOCTRL") != nullptr;
	if (!noCtrl) {
		o.peer = ctrl;
		o.onCreated = [ctrl](rs::DX10Processor& p) { ctrl->connect(&p); };
	}
	// print the level in 100 ms steps for the first 4 s
	o.perBlock = [](rs::DX10Processor& p, double t) {
		static double next = 0.1;
		if (std::getenv("RETRIG_TRACE") && t >= next && t < 4.0) {
			const RVoice& v = p.generators[0].RVoicesMap[kPitch];
			std::printf("    t %.2f  voices %d  env1 %.4f cenv1 %.4f  gainer %.4f  o %.5f  fo %.1f  fc %.1f  ramp %d kill %d  osc1 %.4f\n", t,
			            p.generators[0].notes.size(), v.voiceEnvLfo[1].env, v.voiceEnvLfo[1].cenv, v.gainerTemp, v.o,
			            v.WTOsc.m_dOscFo, v.m_Filter1.m_dFcControl, v.rampingDown ? 1 : 0, v.killIt ? 1 : 0, v.osc1pure);
			next += 0.25;
		}
	};
	std::vector<float> L, R;
	o.captureL = &L;
	o.captureR = &R;
	int ctrlValSnapshot[8] = {};
	o.inspect = [&](rs::DX10Processor& p) {
		const int ids[8] = {OSC_, MOD_OSC_, MOD_TYPE_, OCT_, INT_, MOD_OCT_, PAN_LR_ENV_SRC, MOD_GAIN_ENV_SRC};
		for (int i = 0; i < 8; i++) ctrlValSnapshot[i] = p.generators[0].ctrlValues[ids[i]];
	};
	render(s, ev, seconds, o);
	ctrl->disconnect(nullptr);
	std::printf("switch ints after the controller round trip: OSC %d MOD_OSC %d MOD_TYPE %d OCT %d INT %d MOD_OCT %d PAN_SRC %d MODGAIN_SRC %d\n",
	            ctrlValSnapshot[0], ctrlValSnapshot[1], ctrlValSnapshot[2], ctrlValSnapshot[3], ctrlValSnapshot[4],
	            ctrlValSnapshot[5], ctrlValSnapshot[6], ctrlValSnapshot[7]);

	// per note: the sample-to-sample step (|x[n]-x[n-1]|) is the click detector; with the filter
	// down, normal audio has small steps, a pop has a large one. Look in [on, on + 60 ms].
	struct NoteStat { int idx; double t; float maxStep; int stepAt; float peak; };
	std::vector<NoteStat> st;
	const int w = static_cast<int>(0.060 * kSampleRate);
	int k = 0;
	for (const auto& e : ev) {
		if (!e.on) continue;
		const int on = static_cast<int>(e.sample);
		NoteStat n{k++, e.sample / kSampleRate, 0.0f, 0, 0.0f};
		for (int i = std::max(2, on); i < on + w && i < static_cast<int>(L.size()); i++) {
			// second difference: ~0 for a smooth (filtered) waveform, spikes on a discontinuity
			const float step = std::max(std::fabs(L[i] - 2 * L[i - 1] + L[i - 2]), std::fabs(R[i] - 2 * R[i - 1] + R[i - 2]));
			if (step > n.maxStep) { n.maxStep = step; n.stepAt = i - on; }
			n.peak = std::max(n.peak, std::max(std::fabs(L[i]), std::fabs(R[i])));
		}
		st.push_back(n);
	}
	std::vector<float> steps;
	for (auto& n : st) steps.push_back(n.maxStep);
	std::sort(steps.begin(), steps.end());
	const float med = steps[steps.size() / 2];
	std::printf("retrig probe: filter %.2f, period %.0f ms, gate %.0f ms, %zu notes; median max-step %.5f\n", filter,
	            period * 1000, gate * 1000, st.size(), med);
	std::sort(st.begin(), st.end(), [](const NoteStat& a, const NoteStat& b) { return a.maxStep > b.maxStep; });
	std::printf("  loudest steps (note #, time, step, x median, at ms after note-on, peak in window):\n");
	for (size_t i = 0; i < st.size() && i < 10; i++)
		std::printf("   #%3d  %6.2f s  step %.5f  x%5.1f  at %5.1f ms  peak %.4f\n", st[i].idx, st[i].t, st[i].maxStep,
		            med > 0 ? st[i].maxStep / med : 0.0, st[i].stepAt * 1000.0 / kSampleRate, st[i].peak);
	// dump the worst note's samples around its biggest step for inspection
	if (!st.empty()) {
		const int c = static_cast<int>(st[0].t * kSampleRate) + st[0].stepAt;
		std::printf("  samples around worst step (L, R), n = step position:\n");
		for (int i = c - 6; i <= c + 6; i++)
			if (i > 0 && i < static_cast<int>(L.size()))
				std::printf("    n%+3d  % .5f  % .5f\n", i - c, L[i], R[i]);
	}
	return 0;
}

// Mono (POLY_COUNT 1) steal click probe: each preset plays a phrase of different notes (overlapping
// and detached) once with 1 voice and once with 32. The new notes are identical in both renders, so
// the difference is the stolen voice's fade-out (minus the note it would have kept playing). A jump
// in that difference at the steal is the click. Run: note_off_test.exe mono [filter]
static void renderPresetPoly(int preset, int poly, double filter, const std::vector<NoteEvent>& ev, double seconds,
                             std::vector<float>& L, std::vector<float>& R) {
	const Settings s{"patch", 0.0, 0.0};
	Options o;
	o.keepPatchEnvelopes = true;
	o.params = {{0.0, {rs::BaseController::kPresetParam, (preset + 0.5) / rs::DX10Processor::kNumPrograms}},
	            {0.01, {POLY_COUNT, polyToNormalized(poly)}}};
	if (filter >= 0.0) o.params.push_back({0.02, {FILTER_, filter}});
	static HostApplication chost;
	auto* ctrl = new rs::DX10Controller();
	ctrl->initialize(&chost);
	o.peer = ctrl;
	o.onCreated = [ctrl](rs::DX10Processor& p) { ctrl->connect(&p); };
	// The preset's switch values come back from the controller through the message timer (wall
	// clock): let it run during the first 0.2 s so both renders have the whole preset by the notes.
	o.perBlock = [](rs::DX10Processor&, double t) { if (t < 0.2) { Sleep(20); pumpMessages(); } };
	o.captureL = &L;
	o.captureR = &R;
	render(s, ev, seconds, o);
	ctrl->disconnect(nullptr);
}

struct StealClick { float jump; float level; };  // largest difference in the first 16 samples; old note's level

// One steal per render pair: note `from` (held, or released 50 ms earlier), then note `to`. Up to the
// steal both renders are identical; after it they differ only by the old note's fade-out.
static StealClick monoStealClick(int preset, double filter, int16 from, int16 to, bool held, bool dump) {
	const double tOn = 0.6 + 0.0013 * to;  // vary the position inside the block
	std::vector<NoteEvent> ev = {{at(0.3), true, from}, {at(held ? 0.9 : tOn - 0.05), false, from},
	                             {at(tOn), true, to}, {at(1.0), false, to}};
	std::vector<float> L1, R1, L2, R2;
	renderPresetPoly(preset, 1, filter, ev, 1.3, L1, R1);
	renderPresetPoly(preset, 32, filter, ev, 1.3, L2, R2);
	const int on = static_cast<int>(ev[2].sample);
	StealClick c{0.0f, 0.0f};
	for (int i = on - 64; i < on; i++)
		c.level = std::max(c.level, std::max(std::fabs(L2[i]), std::fabs(R2[i])));
	// A continuous fade (>= 2 ms, 21 ms by default) changes the old note by a few percent in 16
	// samples; a change of its sound at the steal shows at once.
	for (int i = on; i < on + 16; i++)
		c.jump = std::max(c.jump, std::max(std::fabs(L1[i] - L2[i]), std::fabs(R1[i] - R2[i])));
	if (dump) {
		std::printf("  %d -> %d (%s): level %.4f, change %.4f\n", from, to, held ? "held" : "released", c.level, c.jump);
		for (int i = on - 2; i <= on + 6; i++)
			std::printf("     %+4d  poly1 % .4f  poly32 % .4f\n", i - on, L1[i], L2[i]);
	}
	return c;
}

static float monoStealWorst(int preset, double filter, bool dump) {
	static const int16 pairs[][2] = {{55, 60}, {60, 52}, {57, 64}, {48, 55}};
	float worst = 0.0f;
	for (auto& pr : pairs)
		for (int held = 0; held < 2; held++) {
			const StealClick c = monoStealClick(preset, filter, pr[0], pr[1], held != 0, dump);
			if (c.level > 1e-3f)
				worst = std::max(worst, c.jump / c.level);
		}
	return worst;
}

int monoProbe(int argc, char** argv) {
	const double filter = argc > 2 ? std::atof(argv[2]) : -1.0;  // < 0: the preset's own filter
	const char* only = std::getenv("MONO_DUMP");
	for (int p = 0; p < rs::DX10Processor::kNumPrograms; p++) {
		if (only && std::atoi(only) != p) continue;
		std::printf("preset %d: worst change in the first 16 samples after a steal: %.1f %% of the old note's level\n",
		            p, 100.0f * monoStealWorst(p, filter, only != nullptr));
	}
	return 0;
}

// Golden-audio check (optimization safety net): renders every factory preset through the real
// processor + controller path with a fixed note/automation pattern. `golden save` writes the output
// to bin\golden.f32; `golden` compares a new render against it and reports the difference.
int goldenProbe(int argc, char** argv) {
	const bool save = argc > 2 && std::strcmp(argv[2], "save") == 0;
	const double seconds = 3.0;
	std::vector<float> all;
	for (int prog = 0; prog < rs::DX10Processor::kNumPrograms; prog++) {
		std::vector<NoteEvent> ev;
		auto note = [&](double t, double len, int16 p, float vel) {
			// notes start 0.1 s in, after the preset's switch round trip has arrived (as when you
			// pick a preset and then play)
			NoteEvent on{at(t + 0.1), true, p};
			on.velocity = vel;
			ev.push_back(on);
			ev.push_back({at(t + 0.1 + len), false, p});
		};
		note(0.00, 0.80, 48, 1.0f);                       // single note
		note(0.30, 1.20, 60, 0.8f); note(0.30, 1.20, 64, 0.7f); note(0.30, 1.20, 67, 0.6f);  // chord
		for (int k = 0; k < 6; k++) note(1.0 + 0.12 * k, 0.07, 72, 0.5f + 0.08f * k);  // repeats
		note(1.9, 0.9, 36, 0.9f);                         // low note into the tail
		Options o;
		o.keepPatchEnvelopes = true;
		o.params = {{0.0, {rs::BaseController::kPresetParam, (prog + 0.5) / rs::DX10Processor::kNumPrograms}},
		            {1.4, {FILTER_, 0.35}}, {1.8, {FILTER_, 0.8}}, {2.2, {MOD_GAIN_, 0.6}}};
		static HostApplication ghost;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&ghost);
		o.peer = ctrl;
		o.onCreated = [ctrl](rs::DX10Processor& p) { ctrl->connect(&p); };
		// deterministic message delivery: what the 15 ms main-thread timer does, but after every
		// block instead of at wall-clock times (the render runs much faster than real time)
		o.perBlock = [](rs::DX10Processor& p, double) { p.flushMessageQueue(); };
		std::vector<float> L, R;
		o.captureL = &L;
		o.captureR = &R;
		render(Settings{"golden", 0.0, 0.0}, ev, seconds, o);
		ctrl->disconnect(nullptr);
		ctrl->terminate();
		ctrl->release();
		for (size_t i = 0; i < L.size(); i++) { all.push_back(L[i]); all.push_back(R[i]); }
	}
	const char* path = "tests\\note_off\\bin\\golden.f32";
	if (save) {
		FILE* f = std::fopen(path, "wb");
		if (!f) { std::printf("cannot write %s\n", path); return 2; }
		std::fwrite(all.data(), sizeof(float), all.size(), f);
		std::fclose(f);
		std::printf("golden saved: %zu samples (%d presets x %.0f s, stereo)\n", all.size(), rs::DX10Processor::kNumPrograms, seconds);
		return 0;
	}
	std::vector<float> ref(all.size());
	FILE* f = std::fopen(path, "rb");
	if (!f) { std::printf("no golden file; run 'golden save' first\n"); return 2; }
	const size_t got = std::fread(ref.data(), sizeof(float), ref.size(), f);
	std::fclose(f);
	if (got != ref.size()) { std::printf("golden size mismatch\n"); return 2; }
	const size_t perProg = all.size() / rs::DX10Processor::kNumPrograms;
	double worstDb = -400.0;
	float worstAbs = 0.0f;
	for (int prog = 0; prog < rs::DX10Processor::kNumPrograms; prog++) {
		double sig = 0, err = 0;
		float maxAbs = 0;
		for (size_t i = prog * perProg; i < (prog + 1) * perProg; i++) {
			sig += double(ref[i]) * ref[i];
			const double d = double(all[i]) - ref[i];
			err += d * d;
			maxAbs = std::max(maxAbs, static_cast<float>(std::fabs(d)));
		}
		const double db = err > 0 ? 10.0 * std::log10(err / std::max(sig, 1e-30)) : -400.0;
		worstDb = std::max(worstDb, db);
		worstAbs = std::max(worstAbs, maxAbs);
		std::printf("  preset %d: difference %7.1f dB below the signal, max sample diff %.2e\n", prog, -db, maxAbs);
	}
	std::printf("golden: worst preset difference %.1f dB below signal, max sample diff %.2e\n", -worstDb, worstAbs);
	return 0;
}

// Poor man's sampling profiler: a second thread suspends the rendering thread every ~1 ms and
// records its instruction pointer; report() names the functions via DbgHelp (exe built with /DEBUG).
namespace sampler {
std::vector<DWORD64> ips;
volatile bool run = false;
HANDLE target = nullptr;
DWORD WINAPI loop(LPVOID) {
	while (run) {
		if (SuspendThread(target) != (DWORD)-1) {
			CONTEXT ctx{};
			ctx.ContextFlags = CONTEXT_CONTROL;
			if (GetThreadContext(target, &ctx)) ips.push_back(ctx.Rip);
			ResumeThread(target);
		}
		Sleep(1);
	}
	return 0;
}
void report(int top) {
	HANDLE proc = GetCurrentProcess();
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
	SymInitialize(proc, nullptr, TRUE);
	std::vector<std::pair<std::string, int>> hist;
	for (DWORD64 ip : ips) {
		char buf[sizeof(SYMBOL_INFO) + 512];
		auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
		sym->SizeOfStruct = sizeof(SYMBOL_INFO);
		sym->MaxNameLen = 511;
		DWORD64 disp = 0;
		std::string name = SymFromAddr(proc, ip, &disp, sym) ? sym->Name : "?";
		auto it = std::find_if(hist.begin(), hist.end(), [&](const auto& h) { return h.first == name; });
		if (it == hist.end()) hist.push_back({name, 1}); else it->second++;
	}
	std::sort(hist.begin(), hist.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
	std::printf("  profile: %zu samples\n", ips.size());
	for (int i = 0; i < top && i < static_cast<int>(hist.size()); i++)
		std::printf("    %5.1f%%  %s\n", 100.0 * hist[i].second / ips.size(), hist[i].first.c_str());
}
}  // namespace sampler

// Benchmark (not a test): N held voices of the default patch for `seconds`, wall-clock timed.
// Run: note_off_test.exe perf [voices...]   e.g. perf 1 8 32
int perfProbe(int argc, char** argv) {
	std::vector<int> counts;
	for (int i = 2; i < argc; i++) counts.push_back(std::atoi(argv[i]));
	if (counts.empty()) counts = {1, 8, 32};
	const double seconds = std::getenv("PERF_SECONDS") ? std::atof(std::getenv("PERF_SECONDS")) : 5.0;
	const Settings s{"patch", 0.0, 0.0};
	timeBeginPeriod(1);  // 1 ms Sleep resolution for the sampler
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	std::printf("perf: default patch, held notes, %.0f s at 44.1 kHz, %d-sample blocks\n", seconds, kBlock);
	for (int n : counts) {
		std::vector<NoteEvent> ev;
		for (int k = 0; k < n; k++) ev.push_back({0, true, static_cast<int16>(36 + (k % 80))});
		Options o;
		o.keepPatchEnvelopes = true;
		o.params = {{0.0, {ENV_1_DECAY, 1.0}}};  // hold, so all voices stay active
		const bool perfHq = std::getenv("PERF_HQ") != nullptr;  // HQ: voices at 2x (set before activation)
		double t0 = 0, t1 = 0;
		const bool profile = std::getenv("PERF_PROFILE") != nullptr;
		HANDLE th = nullptr;
		o.onCreated = [&](rs::DX10Processor& p) {
			if (perfHq) { p.hqWanted = true; p.floatParams[HQ_MODE] = 1.0; }
			if (profile) {
				sampler::ips.clear();
				DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &sampler::target,
				                THREAD_ALL_ACCESS, FALSE, 0);
				sampler::run = true;
				th = CreateThread(nullptr, 0, sampler::loop, nullptr, 0, nullptr);
			}
			LARGE_INTEGER c; QueryPerformanceCounter(&c); t0 = (double)c.QuadPart;
		};
		o.inspect = [&](rs::DX10Processor& p) {
			LARGE_INTEGER c; QueryPerformanceCounter(&c); t1 = (double)c.QuadPart;
			if (profile) {
				sampler::run = false;
				WaitForSingleObject(th, INFINITE);
				CloseHandle(th);
				CloseHandle(sampler::target);
			}
			std::printf("  %3d voices (active at end: %d): ", n, p.generators[0].notes.size());
		};
		render(s, ev, seconds, o);
		const double secs = (t1 - t0) / f.QuadPart - 0.080;  // render() sleeps 80 ms at the end
		const double perVoiceSample = secs * 1e9 / (seconds * kSampleRate * n);
		std::printf("%.3f s for %.0f s of audio = %.1f%% of one core; %.0f ns per voice-sample; ~%.0f voices fit in real time\n",
		            secs, seconds, 100.0 * secs / seconds, perVoiceSample, (1e9 / kSampleRate) / perVoiceSample);
		if (profile) sampler::report(28);
	}
	return 0;
}

// Alias probe: a high note with heavy FM, normal vs HQ. Prints the share of the spectrum's energy that
// lies away from the note's harmonics (aliasing folds there). Args: alias [pitch] [modGain]
int aliasProbe(int argc, char** argv) {
	const int pitch = argc > 2 ? std::atoi(argv[2]) : 96;
	const double modGain = argc > 3 ? std::atof(argv[3]) : 0.9;
	const int N = 16384;
	std::vector<double> magNormal, magHq;
	for (int hq = 0; hq < 2; hq++) {
		std::vector<float> cap;
		Options o;
		o.captureL = &cap;
		o.params = {{0.0, {MOD_GAIN_, modGain}}, {0.0, {DRIFT_AMOUNT, 0.0}}};
		o.onCreated = [&](rs::DX10Processor& p) { if (hq) { p.hqWanted = true; p.floatParams[HQ_MODE] = 1.0; } };
		render({"alias", 1.0, 0.5}, {{0, true, int16(pitch)}}, 1.2, o);
		// steady part, Hann window, DFT magnitude^2 per bin (direct, N log N not needed here: use Goertzel-like loop)
		const size_t start = cap.size() - N;
		std::vector<double> x(N);
		for (int i = 0; i < N; i++)
			x[i] = cap[start + i] * (0.5 - 0.5 * std::cos(2.0 * 3.141592653589793 * i / (N - 1)));
		// FFT (radix 2)
		std::vector<std::complex<double>> X(N);
		for (int i = 0; i < N; i++) X[i] = x[i];
		for (int i = 1, j = 0; i < N; i++) {
			int bit = N >> 1;
			for (; j & bit; bit >>= 1) j ^= bit;
			j ^= bit;
			if (i < j) std::swap(X[i], X[j]);
		}
		for (int len = 2; len <= N; len <<= 1) {
			const double ang = -2.0 * 3.141592653589793 / len;
			const std::complex<double> wl(std::cos(ang), std::sin(ang));
			for (int i = 0; i < N; i += len) {
				std::complex<double> w(1.0);
				for (int k = 0; k < len / 2; k++) {
					const std::complex<double> u = X[i + k], v = X[i + k + len / 2] * w;
					X[i + k] = u + v;
					X[i + k + len / 2] = u - v;
					w *= wl;
				}
			}
		}
		std::vector<double>& mag = hq ? magHq : magNormal;
		mag.assign(N / 2, 0.0);
		for (int k = 1; k < N / 2; k++) mag[k] = std::abs(X[k]);
	}
	// HQ is the clean reference: what the normal render adds on top of it is mostly aliasing (folded
	// partials); both renders are identical otherwise (same envelopes, levels within 0.02 dB).
	double ref = 0.0, extra = 0.0, extraHigh = 0.0;
	for (int k = 1; k < N / 2; k++) {
		ref += magHq[k] * magHq[k];
		const double d = magNormal[k] > magHq[k] ? magNormal[k] - magHq[k] : 0.0;
		extra += d * d;
		if (k * kSampleRate / N > 5000.0) extraHigh += d * d;
	}
	std::printf("pitch %d, mod gain %.2f: normal adds %.1f dB of energy the HQ render lacks (%.1f dB of it above 5 kHz)\n",
	            pitch, modGain, 10.0 * std::log10((extra > 0 ? extra : 1e-30) / ref), 10.0 * std::log10((extraHigh > 0 ? extraHigh : 1e-30) / ref));
	return 0;
}

// LFO probe: holds a note with LFO 1 routed to the filter and prints LFO 1's per-voice output
// (voiceEnvLfo[4].cenv) after each block. Args: lfo <type 0..1> <rate 0..1> <delay 0..1> <phase 0..1> [amp]
int lfoProbe(int argc, char** argv) {
	const double type = argc > 2 ? std::atof(argv[2]) : 0.0;
	const double rate = argc > 3 ? std::atof(argv[3]) : 0.1;
	const double delay = argc > 4 ? std::atof(argv[4]) : 0.0;
	const double phase = argc > 5 ? std::atof(argv[5]) : 0.5;
	const double amp = argc > 6 ? std::atof(argv[6]) : 1.0;
	Options o;
	o.params = {{0.0, {LFO_1_BPM, 0.0}}, {0.0, {LFO_1_TYPE, type}}, {0.0, {LFO_1_RATE, rate}},
	            {0.0, {LFO_1_DELAY, delay}}, {0.0, {LFO_1_PHASE, phase}}, {0.0, {LFO_1_AMP, amp}},
	            {0.0, {GAIN_ENV, 0.6}}};
	o.onCreated = [](rs::DX10Processor& p) { p.generators[0].ctrlValues[GAIN_ENV_SRC] = 4; };
	o.perBlock = [&](rs::DX10Processor& p, double t) {
		const RVoice& v = p.generators[0].RVoicesMap[kPitch];
		std::printf("%.4f %.4f wf=%u fo=%.3f\n", t, v.voiceEnvLfo[4].cenv, v.voiceEnvLfo[4].m_uWaveform,
		            v.voiceEnvLfo[4].m_dFo);
	};
	render({"lfo", 1.0, 0.5}, {{at(0.05), true, kPitch}}, 2.0, o);
	return 0;
}

int main(int argc, char** argv) {
	if (argc > 1 && std::strcmp(argv[1], "lfo") == 0) return lfoProbe(argc, argv);
	if (argc > 1 && std::strcmp(argv[1], "alias") == 0) return aliasProbe(argc, argv);
	if (argc > 1 && std::strcmp(argv[1], "retrig") == 0) return retrigProbe(argc, argv);
	if (argc > 1 && std::strcmp(argv[1], "mono") == 0) return monoProbe(argc, argv);
	if (argc > 1 && std::strcmp(argv[1], "perf") == 0) return perfProbe(argc, argv);
	if (argc > 1 && std::strcmp(argv[1], "golden") == 0) return goldenProbe(argc, argv);
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

	// ---- B-20: width-env polarity per oscillator ----
	{
		std::printf("\nB-20 width-env polarity (main WIDTH 0.3 -> main pole -1; MOD_WIDTH 0.7 -> mod pole +1)\n");
		float wp = 0, mwp = 0;
		Options o;
		// mod width first, then main width (the main one used to overwrite the modulator's pole)
		o.params = {{0.05, {MOD_WIDTH_, 0.7}}, {0.10, {WIDTH_, 0.3}}};
		o.inspect = [&](rs::DX10Processor& p) { wp = p.generators[0].widthPole; mwp = p.generators[0].modWidthPole; };
		render(settings[0], {}, 0.3, o);
		const bool ok = wp == -1.0f && mwp == 1.0f;
		if (!ok) failures++;
		std::printf("  widthPole %+.0f (want -1)  modWidthPole %+.0f (want +1)  %s\n", wp, mwp, ok ? "PASS" : "FAIL");
	}

	// ---- B-47: switching the mod type mid-note must not leave stale modulation ----
	{
		std::printf("\nB-47 mod type freq-mod -> none while a note is held\n");
		double maxDuring = 0.0, after = -1.0;
		Options o;
		o.perBlock = [&](rs::DX10Processor& p, double t) {
			auto& g = p.generators[0];
			g.ctrlValues[MOD_TYPE_] = t < 0.5 ? 2 : 0;  // 2 = doFreqMod, 0 = doNoMod
			const double v = std::fabs(g.RVoicesMap[kPitch].WTOsc.m_dFoModLin);
			if (t < 0.5) maxDuring = std::max(maxDuring, v); else after = v;
		};
		render(settings[0], {{0, true, kPitch}}, 1.0, o);
		const bool ok = maxDuring > 1.0 && after == 0.0;
		if (!ok) failures++;
		std::printf("  carrier freq offset: max %.1f Hz during freq-mod, %.3f Hz after switching to none  %s\n", maxDuring,
		            after, ok ? "PASS" : "FAIL");
	}

	// ---- B-56: modulation envelopes must not drop to zero on a retrigger ----
	{
		std::printf("\nB-56 env 2 (mod env) level across a retrigger (slow env-2 attack, note re-struck at 1.0 s)\n");
		double before = -1.0, after = -1.0, fresh = -1.0;
		Options o;
		o.params = {{0.0, {ENV_2_ATTACK, 0.6}}, {0.0, {ENV_2_DECAY, 1.0}}};  // slow attack, hold
		o.perBlock = [&](rs::DX10Processor& p, double t) {
			const double c = p.generators[0].RVoicesMap[kPitch].voiceEnvLfo[2].cenv;
			if (t > 0.055 && fresh < 0) fresh = c;   // ~55 ms into the first (fresh) note
			if (t < 1.0) before = c;                  // last block before the re-strike
			if (t > 1.075 && after < 0) after = c;    // ~75 ms after re-strike (fade 21 ms + 50 ms)
		};
		render(settings[0], {{0, true, kPitch}, {at(1.0), true, kPitch}, {at(1.5), false, kPitch}}, 2.0, o);
		const double ratio = before > 0 ? after / before : 0.0;
		// fresh < 0.9 * before: the attack is slow enough to see (a fresh note still starts from 0);
		// the old retrigger restarted env 2 from 0 too, so 50 ms after it the level was back at ~fresh.
		const bool ok = before > 0.5 && ratio > 0.9 && fresh < 0.9 * before;
		if (!ok) failures++;
		std::printf("  env2: fresh note @55 ms %.3f, before re-strike %.3f, 50 ms after restart %.3f (ratio %.2f, want > 0.9)  %s\n",
		            fresh, before, after, ratio, ok ? "PASS" : "FAIL");
	}

	// ---- B-22: oscillator-1 note index must stay inside midiFreqTable[128] ----
	{
		std::printf("\nB-22 oscillator 1 pitch at the keyboard extremes with octave/interval at the limits\n");
		struct PCase { const char* name; int16 pitch; int oct; int intv; double lo, hi; };
		const PCase pc[] = {
			{"MIDI 127, octave +2, interval +12", 127, 5, 12, 5000.0, 13000.0},
			{"MIDI 0, octave -2, interval 0", 0, 1, 0, 1.0, 20.0},
		};
		for (const auto& c : pc) {
			double fo = 0.0;
			Options o;
			o.onCreated = [&](rs::DX10Processor& p) {
				p.generators[0].ctrlValues[OCT_] = c.oct;
				p.generators[0].ctrlValues[INT_] = c.intv;
			};
			o.inspect = [&](rs::DX10Processor& p) { fo = p.generators[0].RVoicesMap[c.pitch].WTOsc.m_dOscFo; };
			render(settings[0], {{0, true, c.pitch}}, 0.3, o);
			const bool ok = std::isfinite(fo) && fo >= c.lo && fo <= c.hi;
			if (!ok) failures++;
			std::printf("  %-36s osc 1 frequency %.2f Hz (want %.0f..%.0f)  %s\n", c.name, fo, c.lo, c.hi, ok ? "PASS" : "FAIL");
		}
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

	// ---- B-08: processor state loading ----
	{
		std::printf("\nB-08 processor setState robustness\n");
		static HostApplication shost;
		auto fresh = [&]() {
			auto* p = new rs::DX10Processor();
			p->initialize(&shost);
			return p;
		};
		auto done = [](rs::DX10Processor* p) { p->terminate(); p->release(); };
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[160];

		// 1. round trip: a saved state loads back exactly (compatibility with existing projects)
		{
			auto* a = fresh();
			a->floatParams[FILTER_] = 0.37;
			a->floatParams[ENV_2_DECAY] = 0.61;
			MemoryStream ms;
			static_cast<IComponent*>(a)->getState(&ms);
			ms.seek(0, IBStream::kIBSeekSet, nullptr);
			auto* b = fresh();
			const tresult r = static_cast<IComponent*>(b)->setState(&ms);
			std::snprintf(buf, sizeof(buf), "result %d, FILTER_ %.3f, ENV_2_DECAY %.3f, bypass %d", (int)r,
			              b->floatParams[FILTER_], b->floatParams[ENV_2_DECAY], b->isBypassed() ? 1 : 0);
			check("round trip (getState -> setState)",
			      r == kResultTrue && std::fabs(b->floatParams[FILTER_] - 0.37) < 1e-9 &&
			          std::fabs(b->floatParams[ENV_2_DECAY] - 0.61) < 1e-9 && !b->isBypassed(), buf);
			done(a);
			done(b);
		}
		// 2. a shorter state (10 params, then bypass = 0): must not read past it or turn bypass on
		{
			MemoryStream ms;
			IBStreamer w(&ms, kLittleEndian);
			w.writeInt32u(rs::BaseController::kMagicNumber);
			w.writeInt32u(2);  // program
			w.writeInt32u(10);  // count
			for (int i = 0; i < 10; i++) w.writeDouble(0.123);
			w.writeInt32u(0);  // bypass off
			ms.seek(0, IBStream::kIBSeekSet, nullptr);
			auto* p = fresh();
			static_cast<IComponent*>(p)->setState(&ms);
			// params after the 10 in the state keep preset 2's values (the old loop read on past the
			// data and IBStreamer stores 0.0 for a failed read, zeroing the rest of the patch)
			const double want = rs::DX10Processor::programParams[2][ENV_1_DECAY];
			std::snprintf(buf, sizeof(buf), "param 0 %.3f, ENV_1_DECAY %.3f (want %.3f), bypass %d", p->floatParams[0],
			              p->floatParams[ENV_1_DECAY], want, p->isBypassed() ? 1 : 0);
			check("short state (10 params)", std::fabs(p->floatParams[0] - 0.123) < 1e-9 &&
			          std::fabs(p->floatParams[ENV_1_DECAY] - want) < 1e-6 && !p->isBypassed(), buf);
			done(p);
		}
		// 3. program number out of range
		{
			MemoryStream ms;
			IBStreamer w(&ms, kLittleEndian);
			w.writeInt32u(rs::BaseController::kMagicNumber);
			w.writeInt32u(50);  // program 50 (only 7 exist)
			w.writeInt32u(0);
			w.writeInt32u(0);
			ms.seek(0, IBStream::kIBSeekSet, nullptr);
			auto* p = fresh();
			static_cast<IComponent*>(p)->setState(&ms);
			std::snprintf(buf, sizeof(buf), "current program %u (want %d)", p->getCurrentProgram(), rs::DX10Processor::kNumPrograms - 1);
			check("program 50 in state", p->getCurrentProgram() == static_cast<uint32>(rs::DX10Processor::kNumPrograms - 1), buf);
			done(p);
		}
		// 4. truncated state (cut in the middle of the params)
		{
			MemoryStream ms;
			IBStreamer w(&ms, kLittleEndian);
			w.writeInt32u(rs::BaseController::kMagicNumber);
			w.writeInt32u(0);
			w.writeInt32u(1000);
			for (int i = 0; i < 3; i++) w.writeDouble(0.5);
			ms.seek(0, IBStream::kIBSeekSet, nullptr);
			auto* p = fresh();
			const tresult r = static_cast<IComponent*>(p)->setState(&ms);
			std::snprintf(buf, sizeof(buf), "result %d (want kResultFalse=%d), bypass %d", (int)r, (int)kResultFalse, p->isBypassed() ? 1 : 0);
			check("truncated state", r == kResultFalse && !p->isBypassed(), buf);
			done(p);
		}
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

	// ---- master volume (MASTER_VOL, 2026-11-14): level, state compatibility, presets ----
	{
		std::printf("\nMaster volume (MASTER_VOL)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[160];
		static HostApplication mhost;
		auto fresh = [&]() {
			auto* p = new rs::DX10Processor();
			p->initialize(&mhost);
			return p;
		};
		auto done = [](rs::DX10Processor* p) { p->terminate(); p->release(); };

		// 1. level: held note, peak over 0.5..1.0 s, at 0 dB / -6 dB (v 0.5) / off (v 0)
		auto heldPeak = [&](double vol, bool setIt) {
			Options o;
			if (setIt) o.params.push_back({0.0, {MASTER_VOL, vol}});
			auto pk = render(Settings{"vol", 0.995, 0.5}, {{0, true, 60}}, 1.0, o);
			float m = 0.0f;
			for (size_t i = 50; i < pk.size(); i++) m = std::max(m, pk[i]);
			return m;
		};
		const float p0 = heldPeak(0.0, false), pHalf = heldPeak(0.5, true), pOff = heldPeak(0.0, true);
		const double ratioDb = 20.0 * std::log10(std::max(1e-9, (double)pHalf / std::max(1e-9f, p0)));
		std::snprintf(buf, sizeof(buf), "default %.4f, v 0.5 %.4f (%+.2f dB, want -6.02), v 0 %.6f", p0, pHalf, ratioDb, pOff);
		check("output level follows the knob", p0 > 0.01f && std::fabs(ratioDb + 6.02) < 0.1 && pOff < 1e-6f, buf);

		// 2. a fresh instance is at 0 dB
		{
			auto* p = fresh();
			std::snprintf(buf, sizeof(buf), "%.6f (want %.6f)", p->floatParams[MASTER_VOL], DEFAULT_MASTER_VOL);
			check("fresh processor at 0 dB", std::fabs(p->floatParams[MASTER_VOL] - DEFAULT_MASTER_VOL) < 1e-6, buf);
			done(p);
		}
		// 3. a state saved before the parameter existed (slot 146 = 0, no tagged block) loads at 0 dB
		auto oldState = [](MemoryStream& ms) {
			IBStreamer w(&ms, kLittleEndian);
			w.writeInt32u(rs::BaseController::kMagicNumber);
			w.writeInt32u(0);
			w.writeInt32u(GPARAMS);
			for (int i = 0; i < GPARAMS; i++) w.writeDouble(i == MASTER_VOL ? 0.0 : rs::DX10Processor::programParams[0][i]);
			w.writeInt32u(0);  // bypass
			ms.seek(0, IBStream::kIBSeekSet, nullptr);
		};
		{
			MemoryStream ms;
			oldState(ms);
			auto* p = fresh();
			static_cast<IComponent*>(p)->setState(&ms);
			std::snprintf(buf, sizeof(buf), "%.6f (want %.6f)", p->floatParams[MASTER_VOL], DEFAULT_MASTER_VOL);
			check("old state (no volume block) loads at 0 dB", std::fabs(p->floatParams[MASTER_VOL] - DEFAULT_MASTER_VOL) < 1e-6, buf);
			done(p);
		}
		// 4. round trip keeps the value; 5. a preset change keeps it
		{
			auto* a = fresh();
			a->floatParams[MASTER_VOL] = 0.42;
			MemoryStream ms;
			static_cast<IComponent*>(a)->getState(&ms);
			ms.seek(0, IBStream::kIBSeekSet, nullptr);
			auto* b = fresh();
			const tresult r = static_cast<IComponent*>(b)->setState(&ms);
			std::snprintf(buf, sizeof(buf), "result %d, %.4f (want 0.4200), bypass %d", (int)r, b->floatParams[MASTER_VOL], b->isBypassed() ? 1 : 0);
			check("round trip keeps the volume", r == kResultTrue && std::fabs(b->floatParams[MASTER_VOL] - 0.42) < 1e-9 && !b->isBypassed(), buf);
			b->setCurrentProgram(3);
			std::snprintf(buf, sizeof(buf), "%.4f after preset 3 (want 0.4200)", b->floatParams[MASTER_VOL]);
			check("preset change keeps the volume (processor)", std::fabs(b->floatParams[MASTER_VOL] - 0.42) < 1e-9, buf);
			done(a);
			done(b);
		}
		// 6. controller: old state -> 0 dB, new state -> value, preset change keeps it, host text in dB
		{
			static HostApplication chost2;
			auto* ctrl = new rs::DX10Controller();
			ctrl->initialize(&chost2);
			const double initV = ctrl->getParamNormalized(MASTER_VOL);
			MemoryStream ms;
			oldState(ms);
			ctrl->setComponentState(&ms);
			const double oldV = ctrl->getParamNormalized(MASTER_VOL);
			auto* a = fresh();
			a->floatParams[MASTER_VOL] = 0.3;
			MemoryStream ms2;
			static_cast<IComponent*>(a)->getState(&ms2);
			ms2.seek(0, IBStream::kIBSeekSet, nullptr);
			ctrl->setComponentState(&ms2);
			const double newV = ctrl->getParamNormalized(MASTER_VOL);
			ctrl->setParamNormalized(rs::BaseController::kPresetParam, (2 + 0.5) / rs::DX10Processor::kNumPrograms);
			const double afterPreset = ctrl->getParamNormalized(MASTER_VOL);
			String128 s0, s1;
			ctrl->getParamStringByValue(MASTER_VOL, DEFAULT_MASTER_VOL, s0);
			ctrl->getParamStringByValue(MASTER_VOL, 0.5, s1);
			char t0[32], t1[32];
			for (int i = 0; i < 32; i++) { t0[i] = (char)s0[i]; t1[i] = (char)s1[i]; if (!s0[i] && !s1[i]) break; }
			t0[31] = t1[31] = 0;
			std::snprintf(buf, sizeof(buf), "init %.3f old %.3f new %.3f preset %.3f, '%s' '%s'", initV, oldV, newV, afterPreset, t0, t1);
			check("controller: default, states, preset, dB text",
			      std::fabs(initV - DEFAULT_MASTER_VOL) < 1e-6 && std::fabs(oldV - DEFAULT_MASTER_VOL) < 1e-6 &&
			          std::fabs(newV - 0.3) < 1e-9 && std::fabs(afterPreset - 0.3) < 1e-9 &&
			          std::strcmp(t0, "0.0") == 0 && std::strcmp(t1, "-6.0") == 0, buf);
			done(a);
			ctrl->terminate();
			ctrl->release();
		}
	}

	// ---- scope (2026-11-14): ScopeCapture alone, then the processor's capture and messages ----
	{
		std::printf("\nScope capture\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		const int L = ScopeCapture::kLength;
		auto risingCrossings = [L](const float* f) {
			int n = 0;
			for (int i = 1; i < L; i++) if (f[i - 1] < 0.0f && f[i] >= 0.0f) n++;
			return n;
		};
		// 1. triggered on a rising zero crossing; 2. nothing new on a second read
		{
			static ScopeCapture sc;
			sc.setSampleRate(44100.0);
			const float* f = nullptr;
			for (int i = 0; i < 20000 && !f; i++) {
				sc.push(static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 220.0 * i / 44100.0 + 1.0)));
				if (i % 300 == 0) f = sc.latest();
			}
			if (!f) f = sc.latest();
			const bool ok = f && f[0] >= 0.0f && f[0] < 0.05f && f[1] > f[0];
			const int cr = f ? risingCrossings(f) : -1;
			std::snprintf(buf, sizeof(buf), "f[0] %.4f f[1] %.4f, %d rising crossings (want 5)", f ? f[0] : -9.f, f ? f[1] : -9.f, cr);
			check("frame starts at a rising zero crossing", ok && cr == 5, buf);
			check("latest() is null until a new frame", sc.latest() == nullptr, "");
		}
		// 3. silence still gives (flat) frames
		{
			static ScopeCapture sc;
			sc.setSampleRate(44100.0);
			for (int i = 0; i < ScopeCapture::kMaxWait + L + 10; i++) sc.push(0.0f);
			const float* f = sc.latest();
			float pk = 0.0f;
			if (f) for (int i = 0; i < L; i++) pk = std::max(pk, std::fabs(f[i]));
			std::snprintf(buf, sizeof(buf), "frame %s, peak %.6f", f ? "yes" : "no", pk);
			check("silence: untriggered flat frame", f && pk == 0.0f, buf);
		}
		// 4. 96 kHz: every 2nd sample (48 kHz), so a frame covers 21.3 ms (4.7 cycles of 220 Hz), not 10.7 ms
		{
			static ScopeCapture sc;
			sc.setSampleRate(96000.0);
			const float* f = nullptr;
			for (int i = 0; i < 40000; i++) {
				sc.push(static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 220.0 * i / 96000.0 + 1.0)));
				if (const float* g = sc.latest()) f = g;
			}
			const int cr = f ? risingCrossings(f) : -1;
			std::snprintf(buf, sizeof(buf), "%d rising crossings (want 4)", cr);
			check("96 kHz frame covers ~21 ms (decimated)", cr == 4, buf);
		}
		// 5. the processor: frames match its output, sent by the timer only (never inside process)
		{
			std::vector<float> capL, capR;
			Options o;
			o.onCreated = [](rs::DX10Processor& p) { p.scopeEnabled.store(true); };
			const float* f = nullptr;
			std::vector<float> frame;
			// take one frame after 0.3 s (perBlock runs before the message pump, so before the timer
			// can take it); the timer sends the others
			o.perBlock = [&](rs::DX10Processor& p, double t) {
				if (frame.empty() && t > 0.3 && (f = p.scope.latest())) frame.assign(f, f + L);
			};
			o.captureL = &capL;
			o.captureR = &capR;
			render(Settings{"scope", 0.995, 0.5}, {{0, true, 57}}, 0.6, o);
			// find the frame in the captured output (mono sum)
			bool found = false;
			if (!frame.empty()) {
				for (size_t s = 0; s + L <= capL.size() && !found; s++) {
					bool same = true;
					for (int i = 0; i < L && same; i++)
						same = std::fabs(0.5f * (capL[s + i] + capR[s + i]) - frame[i]) < 1e-6f;
					found = same;
				}
			}
			const int sentByTimer = g_msgsOut[static_cast<unsigned char>(PMSG_SCOPE_CHAR)];
			const int sentInProcess = g_msgsIn[static_cast<unsigned char>(PMSG_SCOPE_CHAR)];
			std::snprintf(buf, sizeof(buf), "frame %s in output, %d sent by timer, %d inside process", found ? "found" : "NOT found",
			              sentByTimer, sentInProcess);
			check("processor: frame = output, messages from the timer", found && sentInProcess == 0, buf);
		}
		// 6. the controller stores a frame message
		{
			static HostApplication shost3;
			auto* ctrl = new rs::DX10Controller();
			ctrl->initialize(&shost3);
			std::vector<float> f(L);
			for (int i = 0; i < L; i++) f[i] = 0.001f * i;
			HostMessage* m = new HostMessage;
			m->setMessageID("d");
			m->getAttributes()->setBinary("data", f.data(), static_cast<uint32>(L * sizeof(float)));
			ctrl->notify(m);
			m->release();
			std::snprintf(buf, sizeof(buf), "frames %u, last %.3f (want 1, %.3f)", ctrl->scopeFrames, ctrl->scopeFrame[L - 1], f[L - 1]);
			check("controller receives the frame", ctrl->scopeFrames == 1 && ctrl->scopeFrame[L - 1] == f[L - 1], buf);
			ctrl->terminate();
			ctrl->release();
		}
	}

	// ---- voice strip (2026-11-14): per-pitch held/level bytes from the processor ----
	{
		std::printf("\nVoice strip\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		// C-E-G held from 0 s, E released at 0.3 s; look at 0.25 s (all held) and 0.4 s
		uint8 at25[128] = {}, at40[128] = {};
		Options o;
		o.onCreated = [](rs::DX10Processor& p) { p.scopeEnabled.store(true); };
		o.perBlock = [&](rs::DX10Processor& p, double t) {
			uint8* dst = (t >= 0.25 && at25[48] == 0) ? at25 : ((t >= 0.4 && at40[48] == 0) ? at40 : nullptr);
			if (dst) for (int k = 0; k < 128; k++) dst[k] = p.voiceStates[k].load();
		};
		const int64 rel = static_cast<int64>(0.3 * kSampleRate);
		render(Settings{"voices", 0.995, 0.85}, {{0, true, 48}, {0, true, 52}, {0, true, 55}, {rel, false, 52}}, 0.5, o);
		auto others = [](const uint8* a) { int n = 0; for (int k = 0; k < 128; k++) if (k != 48 && k != 52 && k != 55 && a[k]) n++; return n; };
		std::snprintf(buf, sizeof(buf), "0.25 s: %02x %02x %02x, %d other(s) lit", at25[48], at25[52], at25[55], others(at25));
		check("held chord: three held voices, nothing else",
		      (at25[48] & 0x80) && (at25[52] & 0x80) && (at25[55] & 0x80) && (at25[48] & 0x7F) > 0 && others(at25) == 0, buf);
		std::snprintf(buf, sizeof(buf), "0.40 s: %02x %02x %02x (E released)", at40[48], at40[52], at40[55]);
		check("released note shows as releasing (not held)", (at40[48] & 0x80) && (at40[55] & 0x80) && !(at40[52] & 0x80), buf);
		const int sentByTimer = g_msgsOut[static_cast<unsigned char>(PMSG_VOICES_CHAR)];
		const int sentInProcess = g_msgsIn[static_cast<unsigned char>(PMSG_VOICES_CHAR)];
		std::snprintf(buf, sizeof(buf), "%d sent by timer, %d inside process", sentByTimer, sentInProcess);
		check("voice messages from the timer only", sentByTimer > 0 && sentInProcess == 0, buf);
		std::snprintf(buf, sizeof(buf), "%02x %02x %02x %02x", voiceStateByte(true, 1.0f), voiceStateByte(false, 0.00005f),
		              voiceStateByte(false, 0.001f), voiceStateByte(true, 2.0f));
		check("voiceStateByte encoding", voiceStateByte(true, 1.0f) == 0xFF && voiceStateByte(false, 0.00005f) == 0 &&
		          voiceStateByte(false, 0.001f) == 1 && voiceStateByte(true, 2.0f) == 0xFF, buf);
		static HostApplication shost4;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&shost4);
		uint8 st[128] = {};
		st[60] = 0x80 | 100;
		st[64] = 40;
		HostMessage* m = new HostMessage;
		m->setMessageID("f");
		m->getAttributes()->setBinary("data", st, sizeof(st));
		ctrl->notify(m);
		m->release();
		std::snprintf(buf, sizeof(buf), "%02x %02x", ctrl->voiceStates[60], ctrl->voiceStates[64]);
		check("controller receives voice states", ctrl->voiceStates[60] == (0x80 | 100) && ctrl->voiceStates[64] == 40, buf);
		ctrl->terminate();
		ctrl->release();

		// pitch range drawn in the scope: lowest..highest sounding, at least an octave, inside 0..127
		struct RangeCase { std::vector<int> notes; int lastLo, lastHi, wantLo, wantHi; const char* name; };
		const RangeCase cases[] = {
			{{60}, 0, 11, 55, 66, "one note: one octave around it"},
			{{57, 60, 64}, 0, 11, 55, 66, "close chord: one octave"},
			{{45, 57, 69}, 0, 11, 45, 69, "spread: lowest..highest"},
			{{0}, 50, 70, 0, 11, "bottom note: kept inside"},
			{{127}, 50, 70, 116, 127, "top note: kept inside"},
			{{0, 127}, 50, 70, 0, 127, "both ends: all 128"},
			{{}, 40, 63, 40, 63, "nothing sounding: keep last"},
		};
		for (const auto& c : cases) {
			unsigned char s[128] = {};
			for (int n : c.notes) s[n] = 0x80 | 64;
			int lo = -1, hi = -1;
			paramDisplay::voiceRange(s, c.lastLo, c.lastHi, lo, hi);
			std::snprintf(buf, sizeof(buf), "%d..%d (want %d..%d)", lo, hi, c.wantLo, c.wantHi);
			check(c.name, lo == c.wantLo && hi == c.wantHi, buf);
		}
	}

	// ---- Playheads (2026-11-15): the newest voices' times since note-on / release ----
	{
		std::printf("\nPlayheads\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		// five notes 0.1 s apart (one more than the slots), the newest released at 0.4 s, and a
		// retrigger of a sounding pitch (60) at 0.6 s
		struct Snap { double t = -1; PlayheadSlot s[kPlayheadSlots]; };
		Snap a, b, c;
		Options o;
		o.onCreated = [](rs::DX10Processor& p) { p.scopeEnabled.store(true); };
		o.perBlock = [&](rs::DX10Processor& p, double t) {
			Snap* dst = (t >= 0.35 && a.t < 0) ? &a : ((t >= 0.5 && b.t < 0) ? &b : ((t >= 0.7 && c.t < 0) ? &c : nullptr));
			if (!dst) return;
			dst->t = t;
			for (int k = 0; k < kPlayheadSlots; k++)
				dst->s[k] = {p.phPitch[k].load(), p.phAge[k].load(), p.phRel[k].load(), p.phLevel[k].load()};
		};
		render(Settings{"playheads", 0.995, 0.85},
		       {{at(0.0), true, 48}, {at(0.1), true, 52}, {at(0.2), true, 55}, {at(0.25), true, 60}, {at(0.3), true, 64},
		        {at(0.4), false, 64}, {at(0.6), true, 60}}, 0.8, o);
		// a block is 11.6 ms: the snapshot time is the end of the first block past the mark
		const int wantP[4] = {64, 60, 55, 52};
		const double wantOn[4] = {0.3, 0.25, 0.2, 0.1};
		bool ok = true;
		std::string d;
		for (int k = 0; k < 4; k++) {
			const double want = a.t - wantOn[k];
			ok = ok && a.s[k].pitch == wantP[k] && std::fabs(a.s[k].age - want) < 0.002 && a.s[k].relAge < 0;
			char t[32];
			std::snprintf(t, sizeof(t), "%d:%.3f ", a.s[k].pitch, a.s[k].age);
			d += t;
		}
		check("newest four voices, newest first, age since note-on", ok, d.c_str());
		std::snprintf(buf, sizeof(buf), "%d age %.3f rel %.3f (want %.3f / %.3f)", b.s[0].pitch, b.s[0].age, b.s[0].relAge,
		              b.t - 0.3, b.t - 0.4);
		check("released voice: time since release", b.s[0].pitch == 64 && std::fabs(b.s[0].relAge - (b.t - 0.4)) < 0.002 &&
		          std::fabs(b.s[0].age - (b.t - 0.3)) < 0.002 && b.s[0].level > 0.0f, buf);
		// the retrigger (key sync off: a soft re-attack at the note-on) restarts the voice's age
		std::snprintf(buf, sizeof(buf), "%d age %.3f (note-on %.3f s before)", c.s[0].pitch, c.s[0].age, c.t - 0.6);
		check("retriggered voice is newest, age from its restart", c.s[0].pitch == 60 &&
		          std::fabs(c.s[0].age - (c.t - 0.6)) < 0.002 && c.s[0].relAge < 0, buf);
		const int byTimer = g_msgsOut[static_cast<unsigned char>(PMSG_PLAYHEADS_CHAR)];
		const int inProcess = g_msgsIn[static_cast<unsigned char>(PMSG_PLAYHEADS_CHAR)];
		std::snprintf(buf, sizeof(buf), "%d sent by timer, %d inside process", byTimer, inProcess);
		check("playhead messages from the timer only", byTimer > 0 && inProcess == 0, buf);

		static HostApplication shostPh;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&shostPh);
		PlayheadSlot s[kPlayheadSlots] = {{60, 0.5f, -1.0f, 0.8f}, {64, 0.7f, 0.1f, 0.3f}, {-1, 0, -1, 0}, {-1, 0, -1, 0}};
		HostMessage* m = new HostMessage;
		m->setMessageID("k");
		m->getAttributes()->setBinary("data", s, sizeof(s));
		ctrl->notify(m);
		m->release();
		std::snprintf(buf, sizeof(buf), "%d %.2f / %d %.2f, frames %u", ctrl->playheads[0].pitch, ctrl->playheads[0].age,
		              ctrl->playheads[1].pitch, ctrl->playheads[1].relAge, ctrl->playheadFrames);
		check("controller receives playheads", ctrl->playheads[0].pitch == 60 && ctrl->playheads[1].relAge > 0.09f &&
		          ctrl->playheads[2].pitch == -1 && ctrl->playheadFrames == 1, buf);
		ctrl->terminate();
		ctrl->release();
	}

	// ---- LFO shape: the engine's LFO output equals lfoShape (what the LFO displays draw) (2026-11-15) ----
	{
		std::printf("\nLFO shape (engine vs lfoShape.h)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		struct LCase { const char* name; double type, rate, delay, phase, amp; };
		const LCase cases[] = {
			{"sine, phase 0.5", 0.0, 0.1, 0.0, 0.5, 1.0},
			{"sine, phase 0.8, delay 0.25 s", 0.0, 0.1, 0.25, 0.8, 0.7},
			{"saw, phase 0.3", 1.0 / 3, 0.1, 0.0, 0.3, 1.0},
			{"triangle, phase 0.5, delay 0.4 s", 2.0 / 3, 0.15, 0.4, 0.5, 0.5},
			{"square (was silent), phase 0.5", 1.0, 0.1, 0.0, 0.5, 1.0},
			{"square, phase 0.2, delay 0.1 s", 1.0, 0.2, 0.1, 0.2, 0.8},
		};
		const int64 noteAt = at(0.05);
		for (const LCase& c : cases) {
			Options o;
			o.params = {{0.0, {LFO_1_BPM, 0.0}}, {0.0, {LFO_1_TYPE, c.type}}, {0.0, {LFO_1_RATE, c.rate}},
			            {0.0, {LFO_1_DELAY, c.delay}}, {0.0, {LFO_1_PHASE, c.phase}}, {0.0, {LFO_1_AMP, c.amp}},
			            {0.0, {GAIN_ENV, 0.6}}};
			o.onCreated = [](rs::DX10Processor& p) { p.generators[0].ctrlValues[GAIN_ENV_SRC] = 4; };
			const int type = lfoShape::typeFromNormalized(c.type);
			const double start = lfoShape::phaseStart(type, c.phase);
			const double delay = lfoShape::delaySeconds(c.delay);
			const double hz = paramDisplay::lfoRateHz(c.rate);
			int n = 0, bad = 0;
			double worst = 0.0, peak = 0.0;
			o.perBlock = [&](rs::DX10Processor& p, double tEnd) {
				const int64 last = static_cast<int64>(tEnd * kSampleRate + 0.5) - 1;  // last sample of the block
				if (last < noteAt + 64) return;
				const double t = double(last - noteAt) / kSampleRate;
				const double got = p.generators[0].RVoicesMap[kPitch].voiceEnvLfo[4].cenv;
				// skip points next to a jump (saw wrap, square edges, end of the delay): one sample off
				// in time is a full-scale difference there
				const double dt = 1.5e-3;
				const double a = lfoShape::valueAt(type, start, delay, hz, t - dt), b = lfoShape::valueAt(type, start, delay, hz, t + dt);
				if (std::fabs(a - b) > 0.25) return;
				const double want = lfoShape::valueAt(type, start, delay, hz, t) * c.amp;
				const double err = std::fabs(got - want);
				worst = std::max(worst, err);
				peak = std::max(peak, std::fabs(got));
				n++;
				if (err > 0.03) bad++;
			};
			render({"lfo", 1.0, 0.5}, {{noteAt, true, kPitch}}, 1.6, o);
			std::snprintf(buf, sizeof(buf), "%d points, worst %.4f, peak %.2f", n, worst, peak);
			check(c.name, n > 50 && bad == 0 && peak > c.amp * 0.9, buf);
		}

		// the start phase follows a type change made after the phase (it used to keep the old type's)
		Options o;
		o.params = {{0.0, {LFO_1_TYPE, 0.0}}, {0.0, {LFO_1_PHASE, 0.3}}, {0.1, {LFO_1_TYPE, 1.0}}};
		float ps = -9.0f;
		o.inspect = [&](rs::DX10Processor& p) { ps = p.EnvLfoCtrls[4].phaseStart; };
		render({"lfo", 1.0, 0.5}, {}, 0.3, o);
		std::snprintf(buf, sizeof(buf), "phaseStart %.3f (want %.3f)", ps, lfoShape::phaseStart(lfoShape::kSquare, 0.3));
		check("phase recomputed on a type change", std::fabs(ps - lfoShape::phaseStart(lfoShape::kSquare, 0.3)) < 1e-6, buf);

		std::snprintf(buf, sizeof(buf), "%d %d %d %d", lfoShape::typeFromNormalized(0.0), lfoShape::typeFromNormalized(1.0 / 3),
		              lfoShape::typeFromNormalized(2.0 / 3), lfoShape::typeFromNormalized(1.0));
		check("type list maps to sine/saw/tri/square", lfoShape::typeFromNormalized(1.0) == 3 &&
		          lfoShape::typeFromNormalized(2.0 / 3) == 2 && lfoShape::typeFromNormalized(1.0 / 3) == 1, buf);
		const double ladder[] = {0.25, 0.5, 1, 2};
		std::snprintf(buf, sizeof(buf), "%.2f %.2f %.2f", lfoShape::chooseWindow(0, 0.1, ladder, 4),
		              lfoShape::chooseWindow(0.3, 0.483, ladder, 4), lfoShape::chooseWindow(0, 5, ladder, 4));
		check("display window: delay + 2 cycles, capped", lfoShape::chooseWindow(0, 0.1, ladder, 4) == 0.25 &&
		          lfoShape::chooseWindow(0.3, 0.483, ladder, 4) == 2 && lfoShape::chooseWindow(0, 5, ladder, 4) == 2, buf);
		{
			paramDisplay::Context ctx;
			ctx.lfoType = lfoShape::kSine;
			const std::string a = paramDisplay::value(LFO_1_PHASE, 0.625, ctx);
			ctx.lfoType = lfoShape::kSaw;
			const std::string b = paramDisplay::value(LFO_1_PHASE, 0.75, ctx);
			const std::string d = paramDisplay::value(LFO_1_DELAY, 0.02, ctx);
			std::snprintf(buf, sizeof(buf), "%s | %s | %s", a.c_str(), b.c_str(), d.c_str());
			check("readout: phase as start in the cycle, delay off", a == "START 90 deg" && b == "START 90 deg" && d == "OFF", buf);
		}
		{
			// display window: held while editing, crossfade after release, then settled
			lfoShape::ViewWindow v;
			const double f0 = v.update(8.0, "8 S", false, 10.0);
			const double f1 = v.update(0.25, "250 MS", true, 10.1);   // knob held: stays at 8 s
			const double w1 = v.window;
			const double f2 = v.update(0.25, "250 MS", false, 10.2);  // released: fade starts
			const double w2 = v.window, from2 = v.fromWindow;
			const double f3 = v.update(0.25, "250 MS", false, 10.35); // half way
			const double f4 = v.update(0.25, "250 MS", false, 10.6);  // done
			std::snprintf(buf, sizeof(buf), "%.2f %.2f/%.2f %.2f %.2f>%.2f %.2f %.2f %d", f0, f1, w1, f2, from2, w2, f3, f4, int(v.fading));
			check("LFO view: holds while editing, then crossfades", f0 == 1.0 && f1 == 1.0 && w1 == 8.0 && f2 == 0.0 &&
			          from2 == 8.0 && w2 == 0.25 && std::fabs(f3 - 0.5) < 1e-6 && f4 == 1.0 && !v.fading, buf);
		}
	}

	// ---- Polyphony (2026-11-15): POLY_COUNT limit, steal order, global state ----
	{
		std::printf("\nPolyphony\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		auto sounding = [](rs::DX10Processor& p, std::vector<int>& out) {
			out.clear();
			const auto& g = p.generators[0];
			for (int i = 0; i < g.notes.size(); i++)
				if (!g.RVoicesMap[g.notes[i]].killIt) out.push_back(g.notes[i]);
			std::sort(out.begin(), out.end());
		};
		auto list = [](const std::vector<int>& v) {
			std::string s;
			for (int n : v) s += std::to_string(n) + " ";
			return s;
		};
		{  // default
			int def = -1;
			Options o;
			o.inspect = [&](rs::DX10Processor& p) { def = p.generators[0].POLYCOUNT; };
			render({"poly", 1.0, 0.5}, {}, 0.05, o);
			std::snprintf(buf, sizeof(buf), "POLYCOUNT %d", def);
			check("default limit is 32", def == POLY_DEFAULT, buf);
		}
		{  // limit 4, six held notes
			std::vector<int> live;
			int lim = -1;
			Options o;
			o.params = {{0.0, {POLY_COUNT, polyToNormalized(4)}}};
			o.inspect = [&](rs::DX10Processor& p) { sounding(p, live); lim = p.generators[0].POLYCOUNT; };
			std::vector<NoteEvent> ev;
			for (int k = 0; k < 6; k++) ev.push_back({at(0.05 + 0.05 * k), true, int16(48 + 2 * k)});
			render({"poly", 1.0, 0.5}, ev, 0.6, o);
			std::snprintf(buf, sizeof(buf), "limit %d, sounding: %s", lim, list(live).c_str());
			check("limit 4: six notes leave the newest four", lim == 4 && live == std::vector<int>({52, 54, 56, 58}), buf);
		}
		{  // limit 2: a released note is stolen before a held one
			std::vector<int> live;
			Options o;
			o.params = {{0.0, {POLY_COUNT, polyToNormalized(2)}}};
			o.inspect = [&](rs::DX10Processor& p) { sounding(p, live); };
			render({"poly", 1.0, 0.9}, {{at(0.05), true, 48}, {at(0.10), true, 52}, {at(0.20), false, 52}, {at(0.25), true, 55}},
			       0.4, o);
			std::snprintf(buf, sizeof(buf), "sounding: %s", list(live).c_str());
			check("steal: released voice before a held one", live == std::vector<int>({48, 55}), buf);
		}
		{  // limit 2: a note in its attack is not the one stolen
			std::vector<int> live;
			Options o;
			o.params = {{0.0, {POLY_COUNT, polyToNormalized(2)}}};
			o.inspect = [&](rs::DX10Processor& p) { sounding(p, live); };
			render({"poly", 0.6, 0.5}, {{at(0.05), true, 48}, {at(0.5), true, 52}, {at(0.5) + 1024, true, 55}}, 0.7, o);
			std::snprintf(buf, sizeof(buf), "sounding: %s", list(live).c_str());
			check("steal: not the newest note (still in its attack)", live == std::vector<int>({52, 55}), buf);
		}
		{  // global: kept by a preset change, saved in the state, default for old states
			int afterProgram = -1;
			std::vector<char> st;
			Options o;
			o.params = {{0.0, {POLY_COUNT, polyToNormalized(12)}}, {0.1, {rs::BaseController::kPresetParam, 3.0 / 7.0}}};
			o.inspect = [&](rs::DX10Processor& p) {
				afterProgram = p.generators[0].POLYCOUNT;
				MemoryStream ms;
				p.getState(&ms);
				st.assign(ms.getData(), ms.getData() + ms.getSize());
			};
			render({"poly", 1.0, 0.5}, {}, 0.2, o);
			presetTools::State ps;
			const bool parsed = presetTools::parseState(st.data(), st.size(), ps);
			std::snprintf(buf, sizeof(buf), "after preset %d; state poly %d (block %d)", afterProgram,
			              ps.hasPoly ? polyFromNormalized(ps.poly) : -1, int(ps.hasPoly));
			check("preset change keeps it; state has a POLY block", afterProgram == 12 && parsed && ps.hasPoly &&
			          polyFromNormalized(ps.poly) == 12, buf);
			// loading: with the block -> that value; an older state (no block) -> the default
			int withBlock = -1, without = -1;
			for (int pass = 0; pass < 2; pass++) {
				presetTools::State s2 = ps;
				if (pass == 1) s2.hasPoly = false;
				s2.values[POLY_COUNT] = 0.0;  // older states hold 0 in this slot
				const std::vector<char> blob = presetTools::writeState(s2);
				Options o2;
				o2.inspect = [&](rs::DX10Processor& p) {
					MemoryStream ms(const_cast<char*>(blob.data()), TSize(blob.size()));
					static_cast<IComponent&>(p).setState(&ms);
					(pass == 0 ? withBlock : without) = p.generators[0].POLYCOUNT;
				};
				render({"poly", 1.0, 0.5}, {}, 0.05, o2);
			}
			std::snprintf(buf, sizeof(buf), "with block %d, old state %d", withBlock, without);
			check("state load: block value, else the default", withBlock == 12 && without == POLY_DEFAULT, buf);
		}
		{  // host text and readout
			static HostApplication shostPoly;
			auto* ctrl = new rs::DX10Controller();
			ctrl->initialize(&shostPoly);
			String128 t = {};
			Parameter* pp = ctrl->getParameterObject(POLY_COUNT);
			std::string txt;
			if (pp) {
				pp->toString(polyToNormalized(8), t);
				char a[64] = {};
				UString(t, 128).toAscii(a, 64);
				txt = a;
			}
			const std::string ro = paramDisplay::value(POLY_COUNT, polyToNormalized(1), paramDisplay::Context());
			const double def = pp ? pp->getInfo().defaultNormalizedValue : -1;
			std::snprintf(buf, sizeof(buf), "host '%s', readout '%s', steps %d, default %d", txt.c_str(), ro.c_str(),
			              pp ? int(pp->getInfo().stepCount) : -1, polyFromNormalized(def));
			check("host text, readout, 64 steps, default 32", txt == "8" && ro == "1 VOICE" && pp &&
			          pp->getInfo().stepCount == POLY_MAX - 1 && polyFromNormalized(def) == POLY_DEFAULT, buf);
			ctrl->terminate();
			ctrl->release();
		}
		{  // B-64: a steal (every new note with 1 voice) fades the old note out; its sound must not jump
			float worst = 0.0f;
			int worstP = -1;
			for (int p = 0; p < rs::DX10Processor::kNumPrograms; p++) {
				const float w = monoStealWorst(p, -1.0, false);
				if (w > worst) { worst = w; worstP = p; }
			}
			std::snprintf(buf, sizeof(buf), "worst %.1f %% of the old note in 16 samples (preset %d)", 100.0f * worst, worstP);
			check("B-64: 1 voice, new note: no jump in the old note", worst < 0.06f, buf);
		}
	}

	// ---- OSC KEY SYNC (2026-11-17): what a repeated pitch does ----
	// off (default): the running voice re-attacks at once (no fade, no gap, nothing reset);
	// on: fade over the Snap time (a gain after the filter), then restart from a reset voice.
	// Click measure: the largest sample-to-sample step around the repeat over the largest the note
	// makes by itself before and after (a jump in the signal shows as a larger step). ENV_1 attack 0.3
	// (~4 ms): the tests' default attack 0 (0.35 ms) is a deliberate hard edge on any re-attack. (The
	// re-attack does bend the slope: a one-pole attack starts at full speed, like an analog envelope.)
	{
		std::printf("\nOSC KEY SYNC (repeated pitch)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		const double tRep = 0.3;  // the repeat; the first note at 0.05 s, held (decaying)
		struct Rep { std::vector<float> L; bool fadingAfter = false; };
		auto rep = [&](bool sync, double snapNorm, double cutoff, double q) {
			Rep r;
			Options o;
			o.params = {{0.0, {KEY_SYNC, sync ? 1.0 : 0.0}}, {0.0, {SNAP_TIME, snapNorm}}, {0.0, {ENV_1_ATTACK, 0.3}}};
			if (cutoff >= 0.0)
				o.params.push_back({0.0, {FILTER_, cutoff}}), o.params.push_back({0.0, {FILTER_Q, q}});
			o.captureL = &r.L;
			o.perBlock = [&](rs::DX10Processor& p, double t) {
				if (t >= tRep && t - tRep < 0.0116)
					r.fadingAfter = p.generators[0].RVoicesMap[kPitch].rampingDown;
			};
			render({"keysync", 0.95, 0.5}, {{at(0.05), true, kPitch}, {at(tRep), true, kPitch}}, 0.5, o);
			return r;
		};
		auto maxStep = [](const std::vector<float>& L, double t0, double t1) {
			float m = 0.0f;
			for (int64 i = std::max<int64>(1, at(t0)); i < at(t1) && i < int64(L.size()); i++)
				m = std::max(m, std::fabs(L[i] - L[i - 1]));
			return m;
		};
		auto rms = [](const std::vector<float>& L, double t0, double t1) {
			double e = 0.0;
			int64 n = 0;
			for (int64 i = at(t0); i < at(t1) && i < int64(L.size()); i++, n++)
				e += double(L[i]) * L[i];
			return n ? float(std::sqrt(e / n)) : 0.0f;
		};
		auto peak = [](const std::vector<float>& L, double t0, double t1) {
			float m = 0.0f;
			for (int64 i = at(t0); i < at(t1) && i < int64(L.size()); i++)
				m = std::max(m, std::fabs(L[i]));
			return m;
		};
		auto click = [&](const Rep& r, double fadeMs) {
			const float ref = std::max(maxStep(r.L, 0.20, tRep - 0.002), maxStep(r.L, tRep + fadeMs / 1000.0 + 0.03, 0.48));
			return ref > 0.0f ? maxStep(r.L, tRep - 0.001, tRep + fadeMs / 1000.0 + 0.01) / ref : 99.0f;
		};
		const double snapDef = snapToNormalized(SNAP_DEFAULT_MS);
		{  // off: immediate, no gap, no click
			const Rep r = rep(false, snapDef, -1.0, 0.0);
			const float before = rms(r.L, tRep - 0.025, tRep), after = rms(r.L, tRep, tRep + 0.025);  // the patch wobbles ~25 ms
			const float c = click(r, 0.0);
			std::snprintf(buf, sizeof(buf), "fading %d, RMS 25 ms after/before %.0f %%, click x%.2f", int(r.fadingAfter),
			              before > 0 ? 100.0f * after / before : 0.0f, c);
			check("off: re-attack at once, no gap, no click", !r.fadingAfter && after >= 0.95f * before && c < 1.5f, buf);
		}
		{  // on: fades to silence, then restarts; no click at either end
			const Rep r = rep(true, snapDef, -1.0, 0.0);
			const double end = tRep + SNAP_DEFAULT_MS / 1000.0;
			const float c = click(r, SNAP_DEFAULT_MS);
			const float tail = peak(r.L, end - 0.0005, end - 0.00005), before = peak(r.L, tRep - 0.025, tRep);
			std::snprintf(buf, sizeof(buf), "fading %d, last 0.5 ms of fade %.2f %% of before, click x%.2f", int(r.fadingAfter),
			              before > 0 ? 100.0f * tail / before : 0.0f, c);
			check("on: fade to silence, restart, no click", r.fadingAfter && tail < 0.01f * before && c < 1.5f, buf);
		}
		// resonant low filter (rings): the fade is after the filter, so the ringing fades too and the
		// filter reset at the restart cannot click
		for (int sync = 0; sync < 2; sync++) {
			const Rep r = rep(sync != 0, snapDef, 0.25, 1.0);
			const float c = click(r, sync ? SNAP_DEFAULT_MS : 0.0);
			// on: the ringing is faded too, so the voice is silent when the filter is reset
			const double end = tRep + SNAP_DEFAULT_MS / 1000.0;
			const float tail = peak(r.L, end - 0.0005, end - 0.00005), before = peak(r.L, tRep - 0.025, tRep);
			std::snprintf(buf, sizeof(buf), sync ? "click x%.2f, last 0.5 ms of fade %.2f %%" : "click x%.2f", c,
			              before > 0 ? 100.0f * tail / before : 0.0f);
			check(sync ? "on, resonant filter: silent at reset, no click" : "off, resonant filter: no click",
			      c < 1.5f && (!sync || tail < 0.01f * before), buf);
		}
		{  // parameter: readout name, host parameter, part of the preset, old states load it off
			const std::string n = paramDisplay::label(KEY_SYNC);
			int n2 = 0;
			const NewParamDefault* d = newParamDefaults(n2);
			bool listed = false;
			for (int i = 0; i < n2; i++)
				if (d[i].pid == KEY_SYNC && d[i].normalized == 0.0 && d[i].since == 3 && STATE_VERSION >= 3) listed = true;
			std::snprintf(buf, sizeof(buf), "name '%s', default off for states before v3 %d", n.c_str(), int(listed));
			check("name, host param, state default", n == "OSC KEY SYNC" && listed && isHostParam(KEY_SYNC) && !isGlobalParam(KEY_SYNC), buf);
		}
	}

	// ---- Snap (2026-11-15): retrigger fade time, and the state version for new parameters ----
	{
		std::printf("\nSnap (retrigger fade) and state version\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		std::snprintf(buf, sizeof(buf), "%.2f %.2f %.2f ms", snapMs(0.0), snapMs(1.0), snapMs(snapToNormalized(SNAP_DEFAULT_MS)));
		check("mapping 2..80 ms, default 21", std::fabs(snapMs(0.0) - 2.0) < 1e-9 && std::fabs(snapMs(1.0) - 80.0) < 1e-9 &&
		          std::fabs(snapMs(snapToNormalized(21.0)) - 21.0) < 1e-9, buf);
		// replay a held note at 0.3 s (hold decay, so only the snap ramp lowers it); still fading at t1?
		auto fading = [&](double snapNorm, double t1) {
			bool f = false;
			Options o;
			o.params = {{0.0, {SNAP_TIME, snapNorm}}, {0.0, {KEY_SYNC, 1.0}}};  // the fade is the key-sync retrigger
			o.perBlock = [&](rs::DX10Processor& p, double t) {
				if (t >= t1 && t - t1 < 0.0116)
					f = p.generators[0].RVoicesMap[kPitch].rampingDown;
			};
			render({"snap", 1.0, 0.5}, {{at(0.05), true, kPitch}, {at(0.3), true, kPitch}}, t1 + 0.05, o);  // held, replayed (no release)
			return f;
		};
		const bool long50 = fading(1.0, 0.35), long110 = fading(1.0, 0.41), short12 = fading(0.0, 0.312);
		std::snprintf(buf, sizeof(buf), "80 ms: fading at +50 ms %d, +110 ms %d; 2 ms: at +12 ms %d", int(long50), int(long110), int(short12));
		check("fade lasts the snap time", long50 && !long110 && !short12, buf);

		// state version: an old state (no SVER block, 0 in the snap slot) loads at 21 ms
		std::vector<char> st;
		float fadeAfterProgram = 0.0f;
		{
			Options o;
			o.params = {{0.0, {SNAP_TIME, 0.9}}};
			o.inspect = [&](rs::DX10Processor& p) {
				MemoryStream ms;
				p.getState(&ms);
				st.assign(ms.getData(), ms.getData() + ms.getSize());
			};
			render({"snap", 1.0, 0.5}, {}, 0.05, o);
		}
		presetTools::State ps;
		presetTools::parseState(st.data(), st.size(), ps);
		float withVersion = 0.0f, oldState = 0.0f;
		for (int pass = 0; pass < 2; pass++) {
			presetTools::State s2 = ps;
			if (pass == 1) {
				s2.version = 0;            // written without the block, like an older build
				s2.values[SNAP_TIME] = 0.0;  // and its slot held 0
			}
			const std::vector<char> blob = presetTools::writeState(s2);
			Options o;
			o.inspect = [&](rs::DX10Processor& p) {
				MemoryStream ms(const_cast<char*>(blob.data()), TSize(blob.size()));
				static_cast<IComponent&>(p).setState(&ms);
				(pass == 0 ? withVersion : oldState) = p.generators[0].retrigFadeMs;
			};
			render({"snap", 1.0, 0.5}, {}, 0.05, o);
		}
		{
			Options o;
			o.params = {{0.0, {SNAP_TIME, 0.0}}, {0.02, {rs::BaseController::kPresetParam, 2.0 / 7.0}}};
			o.inspect = [&](rs::DX10Processor& p) { fadeAfterProgram = p.generators[0].retrigFadeMs; };
			render({"snap", 1.0, 0.5}, {}, 0.05, o);
		}
		std::snprintf(buf, sizeof(buf), "state v%u: %.1f ms; old state %.1f ms; program change %.1f ms", ps.version, withVersion,
		              oldState, fadeAfterProgram);
		check("new state keeps it, old state/preset -> 21 ms", ps.version == STATE_VERSION &&
		          std::fabs(withVersion - snapMs(0.9)) < 0.01 && std::fabs(oldState - 21.0f) < 0.01 &&
		          std::fabs(fadeAfterProgram - 21.0f) < 0.01, buf);
		// controller: same rule for an old state
		{
			static HostApplication shostSnap;
			auto* ctrl = new rs::DX10Controller();
			ctrl->initialize(&shostSnap);
			presetTools::State s2 = ps;
			s2.version = 0;
			s2.values[SNAP_TIME] = 0.0;
			const std::vector<char> blob = presetTools::writeState(s2);
			MemoryStream ms(const_cast<char*>(blob.data()), TSize(blob.size()));
			ctrl->setComponentState(&ms);
			const double v = ctrl->getParamNormalized(SNAP_TIME);
			std::snprintf(buf, sizeof(buf), "%.1f ms", snapMs(v));
			check("controller: old state -> 21 ms", std::fabs(snapMs(v) - 21.0) < 0.01, buf);
			ctrl->terminate();
			ctrl->release();
		}
	}

	// ---- Expression (2026-11-15): EXPR_AMOUNT / EXPR_TARGET / EXPR_VEL ----
	{
		std::printf("\nExpression\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		auto renderExpr = [&](std::vector<std::pair<double, std::pair<ParamID, double>>> extra, std::vector<float>& outL,
		                      std::vector<NoteEvent> ev = {}) {
			Options o;
			o.params = {{0.0, {FILTER_, 0.3}}};
			for (auto& e : extra) o.params.push_back(e);
			o.captureL = &outL;
			if (ev.empty()) ev = {{at(0.05), true, 48}};
			render({"expr", 1.0, 0.5}, ev, 0.5, o);
		};
		std::vector<float> base, centre;
		renderExpr({}, base);
		renderExpr({{0.0, {EXPR_AMOUNT, 0.5}}, {0.0, {EXPR_TARGET, 0.0}}}, centre);
		size_t diff = 0;
		for (size_t i = 0; i < base.size() && i < centre.size(); i++)
			if (base[i] != centre[i]) diff++;
		std::snprintf(buf, sizeof(buf), "%zu samples differ", diff);
		check("centre = no change (bit exact)", diff == 0 && base.size() == centre.size(), buf);
		// the voice's filter coefficient g = tan(pi fc / fs) at the end: higher = brighter (this patch is
		// close to a sine, so the output's own brightness hardly shows a low-pass)
		auto filterG = [&](double amount) {
			double g = 0.0;
			Options o;
			o.params = {{0.0, {FILTER_, 0.3}}, {0.0, {EXPR_AMOUNT, amount}}, {0.0, {EXPR_TARGET, 1.0 / 3}}};
			o.inspect = [&](rs::DX10Processor& p) { g = p.generators[0].RVoicesMap[48].m_Filter1.g; };
			render({"expr", 1.0, 0.5}, {{at(0.05), true, 48}}, 0.3, o);
			return g;
		};
		const double gDown = filterG(0.1), gOff = filterG(0.5), gUp = filterG(0.9);
		std::snprintf(buf, sizeof(buf), "filter g down %.4f < off %.4f < up %.4f", gDown, gOff, gUp);
		check("filter target: amount moves the cutoff", gDown < gOff && gOff < gUp, buf);

		// velocity mode: each voice's offset follows its own velocity
		float soft = -1.0f, hard = -1.0f;
		{
			Options o;
			o.params = {{0.0, {EXPR_AMOUNT, 1.0}}, {0.0, {EXPR_TARGET, 1.0 / 3}}, {0.0, {EXPR_VEL, 1.0}}};
			o.inspect = [&](rs::DX10Processor& p) {
				soft = p.generators[0].RVoicesMap[48].exprFilter;
				hard = p.generators[0].RVoicesMap[55].exprFilter;
			};
			NoteEvent a{at(0.05), true, 48};
			a.velocity = 0.25f;
			NoteEvent b{at(0.05), true, 55};
			b.velocity = 1.0f;
			render({"expr", 1.0, 0.5}, {a, b}, 0.2, o);
		}
		const float wantSoft = 0.5f * (float(int(0.25f * 127)) / 127.0f), wantHard = 0.5f;
		std::snprintf(buf, sizeof(buf), "soft %.4f (want %.4f), hard %.4f (want %.4f)", soft, wantSoft, hard, wantHard);
		check("velocity mode: offset x note velocity", std::fabs(soft - wantSoft) < 1e-3 && std::fabs(hard - wantHard) < 1e-3, buf);

		paramDisplay::Context c;
		c.exprTarget = kExprBright;
		c.exprVel = true;
		const std::string t1 = paramDisplay::value(EXPR_AMOUNT, 0.7, c), t2 = paramDisplay::value(EXPR_AMOUNT, 0.5, c),
		                  t3 = paramDisplay::value(EXPR_TARGET, 1.0, c);
		std::snprintf(buf, sizeof(buf), "'%s' '%s' '%s'", t1.c_str(), t2.c_str(), t3.c_str());
		check("readout", t1 == "+40 % BRIGHT x VEL" && t2 == "OFF" && t3 == "WIDTH", buf);
	}

	// ---- Drift (2026-11-15): per-note random pitch/filter/pan offsets ----
	{
		std::printf("\nDrift\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		std::vector<NoteEvent> chord;
		for (int k = 0; k < 8; k++) chord.push_back({at(0.05), true, int16(48 + k * 3)});
		std::vector<float> off1, zero;
		{
			Options o;
			o.captureL = &off1;
			render({"drift", 1.0, 0.5}, chord, 0.3, o);
			Options o2;
			o2.params = {{0.0, {DRIFT_AMOUNT, 0.0}}};
			o2.captureL = &zero;
			render({"drift", 1.0, 0.5}, chord, 0.3, o2);
		}
		check("off = no change (bit exact)", off1 == zero, off1 == zero ? "identical" : "differs");

		struct D { float p, f, n; };
		auto offsets = [&](std::vector<D>& out, std::vector<float>* cap) {
			Options o;
			o.params = {{0.0, {DRIFT_AMOUNT, 1.0}}};
			o.captureL = cap;
			o.inspect = [&](rs::DX10Processor& p) {
				out.clear();
				for (int k = 0; k < 8; k++) {
					const RVoice& v = p.generators[0].RVoicesMap[48 + k * 3];
					out.push_back({v.driftPitch, v.driftFilter, v.driftPan});
				}
			};
			render({"drift", 1.0, 0.5}, chord, 0.3, o);
		};
		std::vector<D> a, b;
		std::vector<float> capA, capB;
		offsets(a, &capA);
		offsets(b, &capB);
		bool inRange = true, allDiffer = true;
		float maxC = 0.0f;
		for (size_t i = 0; i < a.size(); i++) {
			const float cents = 1200.0f * std::log2(a[i].p);
			maxC = std::max(maxC, std::fabs(cents));
			inRange = inRange && std::fabs(cents) <= kDriftCents + 1e-3f && std::fabs(a[i].f) <= kDriftFilter + 1e-6f &&
			          std::fabs(a[i].n) <= kDriftPan + 1e-6f;
			for (size_t j = 0; j < i; j++)
				if (a[i].p == a[j].p) allDiffer = false;
		}
		std::snprintf(buf, sizeof(buf), "max %.1f cents, first %+.1f c %+.3f f %+.2f pan", maxC, 1200.0f * std::log2(a[0].p), a[0].f, a[0].n);
		check("full drift: offsets in range, each note its own", inRange && allDiffer && maxC > 1.0f, buf);
		check("same input renders the same (deterministic)", capA == capB, capA == capB ? "identical" : "differs");
		// a key-sync retrigger picks new offsets; a soft one (key sync off) keeps them (no pitch step)
		for (int sync = 1; sync >= 0; sync--) {
			float p1 = 0.0f, p2 = 0.0f;
			Options o;
			o.params = {{0.0, {DRIFT_AMOUNT, 1.0}}, {0.0, {KEY_SYNC, double(sync)}}};
			o.perBlock = [&](rs::DX10Processor& p, double t) {
				const float v = p.generators[0].RVoicesMap[kPitch].driftPitch;
				if (t > 0.1 && t < 0.12) p1 = v;
				if (t > 0.3) p2 = v;
			};
			render({"drift", 1.0, 0.5}, {{at(0.05), true, kPitch}, {at(0.2), true, kPitch}}, 0.35, o);
			std::snprintf(buf, sizeof(buf), "%+.2f c then %+.2f c", 1200.0f * std::log2(p1), 1200.0f * std::log2(p2));
			if (sync)
				check("key-sync retrigger: new offsets", p1 != p2 && p1 != 0.0f && p2 != 0.0f, buf);
			else
				check("soft retrigger: same offsets", p1 == p2 && p1 != 0.0f, buf);
		}
		const std::string r = paramDisplay::value(DRIFT_AMOUNT, 0.4, paramDisplay::Context());
		check("readout", r == "40 % (+-6 CENTS)", r.c_str());
	}

	// ---- HQ mode and the engine rate (B-63) (2026-11-15) ----
	{
		std::printf("\nHQ (2x) and sample rate (B-63)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[220];
		// one note, decay 0.6 (no hold): the time the 10 ms peak level takes to fall to half of its maximum
		struct R { double halfTime; float peak; double rms; float genRate; double phAge; };
		auto run = [&](double sr, bool hq) {
			R r{-1.0, 0.0f, 0.0, 0.0f, -1.0};
			Options o;
			o.sampleRate = sr;
			o.params = {{0.0, {HQ_MODE, hq ? 1.0 : 0.0}}};
			std::vector<float> cap;
			o.captureL = &cap;
			o.onCreated = [](rs::DX10Processor& p) { p.scopeEnabled.store(true); };
			o.perBlock = [&](rs::DX10Processor& p, double t) {
				r.genRate = p.generators[0].sampleRate;
				if (r.phAge < 0.0 && t >= 0.3 && p.phPitch[0].load() >= 0)
					r.phAge = p.phAge[0].load() - (t - 0.05);  // playhead age error at this block
			};
			const std::vector<float> peaks = render({"hq", 0.6, 0.5}, {{static_cast<int64>(0.05 * sr), true, 60}}, 1.5, o);
			size_t imax = 0;
			for (size_t i = 0; i < peaks.size(); i++)
				if (peaks[i] > r.peak) { r.peak = peaks[i]; imax = i; }
			for (size_t i = imax; i < peaks.size(); i++)  // first 10 ms window after the peak below half of it
				if (peaks[i] < 0.5f * r.peak) { r.halfTime = i * 0.010; break; }
			double s = 0.0;
			for (float v : cap) s += double(v) * v;
			r.rms = std::sqrt(s / (cap.empty() ? 1 : cap.size()));
			return r;
		};
		const R a = run(44100.0, false), b = run(96000.0, false), c = run(44100.0, true), d = run(96000.0, true);
		std::snprintf(buf, sizeof(buf), "half level at %.3f s (44.1k), %.3f s (96k)", a.halfTime, b.halfTime);
		check("B-63: envelope time independent of the host rate", a.halfTime > 0 && std::fabs(a.halfTime - b.halfTime) <= 0.011, buf);
		std::snprintf(buf, sizeof(buf), "engine %.0f / %.0f Hz; half level %.2f / %.2f s", c.genRate, d.genRate, c.halfTime, d.halfTime);
		check("HQ: engine at 2x, same envelope time", c.genRate == 88200.0f && d.genRate == 192000.0f &&
		          std::fabs(c.halfTime - a.halfTime) <= 0.02 && std::fabs(d.halfTime - a.halfTime) <= 0.02, buf);
		const double db = 20.0 * std::log10(c.rms / a.rms);
		std::snprintf(buf, sizeof(buf), "rms %.4f vs %.4f (%+.2f dB), peak %.3f vs %.3f", c.rms, a.rms, db, c.peak, a.peak);
		check("HQ: same level (decimator gain 1)", std::fabs(db) < 0.5, buf);
		std::snprintf(buf, sizeof(buf), "playhead age error %.4f s (normal %.4f s)", c.phAge, a.phAge);
		check("HQ: playhead clock in seconds", std::fabs(c.phAge) < 0.015 && std::fabs(a.phAge) < 0.015, buf);
		// global: kept across a preset change and saved in its own block
		int hqAfter = -1;
		std::vector<char> st;
		Options o;
		o.params = {{0.0, {HQ_MODE, 1.0}}, {0.05, {rs::BaseController::kPresetParam, 4.0 / 7.0}}};
		o.inspect = [&](rs::DX10Processor& p) {
			hqAfter = p.hqActive ? 1 : 0;
			MemoryStream ms;
			p.getState(&ms);
			st.assign(ms.getData(), ms.getData() + ms.getSize());
		};
		render({"hq", 1.0, 0.5}, {}, 0.2, o);
		presetTools::State ps;
		presetTools::parseState(st.data(), st.size(), ps);
		std::snprintf(buf, sizeof(buf), "after preset change %d; state block %d = %.0f", hqAfter, int(ps.hasHq), ps.hq);
		check("HQ is global (preset change keeps it, saved)", hqAfter == 1 && ps.hasHq && ps.hq == 1.0, buf);
	}

	// ---- FX strip (2026-11-15): chorus and ping-pong delay ----
	{
		std::printf("\nFX strip (chorus, ping-pong delay)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		const std::vector<NoteEvent> blip = {{at(0.05), true, 60}, {at(0.07), false, 60}};
		auto renderFx = [&](std::vector<std::pair<double, std::pair<ParamID, double>>> params, std::vector<float>& L,
		                    std::vector<float>& R) {
			Options o;
			o.params = params;
			o.captureL = &L;
			o.captureR = &R;
			render({"fx", 1.0, 0.1}, blip, 1.4, o);
		};
		std::vector<float> l0, r0, l1, r1;
		renderFx({}, l0, r0);
		renderFx({{0.0, {DLY_FDBK, 0.9}}, {0.0, {CHO_DEPTH, 1.0}}, {0.0, {CHO_RATE, 1.0}}, {0.0, {DLY_TIME, 0.0}}}, l1, r1);
		check("mixes at 0: no change (bit exact)", l0 == l1 && r0 == r1, (l0 == l1 && r0 == r1) ? "identical" : "differs");

		// delay 1/4 at 120 BPM = 0.5 s, mix 100 %, feedback 50 %: echoes left at +0.5 s, right at +1.0 s
		std::vector<float> l2, r2;
		renderFx({{0.0, {DLY_TIME, 8.0 / 11.0}}, {0.0, {DLY_MIX, 1.0}}, {0.0, {DLY_FDBK, 0.5 / 0.95}}, {0.0, {DLY_TONE, 1.0}}}, l2, r2);
		auto peakIn = [&](const std::vector<float>& v, double t0, double t1) {
			float p = 0.0f;
			for (int64 i = at(t0); i < at(t1) && i < int64(v.size()); i++) p = std::max(p, std::fabs(v[size_t(i)]));
			return p;
		};
		const float dry = peakIn(l2, 0.05, 0.2);
		const float gapL = peakIn(l2, 0.3, 0.5), echoL = peakIn(l2, 0.54, 0.7), echoR1 = peakIn(r2, 0.54, 0.7);
		const float echoR = peakIn(r2, 1.04, 1.2), echoL2 = peakIn(l2, 1.04, 1.2);
		std::snprintf(buf, sizeof(buf), "dry %.3f, gap %.4f, L@0.55 %.3f (R %.3f), R@1.05 %.3f (L %.3f)", dry, gapL, echoL, echoR1,
		              echoR, echoL2);
		check("ping-pong: L at one division, then R", dry > 0.05f && gapL < 0.01f * dry && echoL > 0.3f * dry &&
		          echoR1 < 0.1f * echoL && echoR > 0.2f * echoL && echoR < echoL && echoL2 < 0.1f * echoR, buf);

		// chorus: changes the sound, keeps the level
		std::vector<float> l3, r3;
		renderFx({{0.0, {CHO_MIX, 1.0}}, {0.0, {CHO_DEPTH, 1.0}}}, l3, r3);
		double e0 = 0.0, e3 = 0.0, diff = 0.0;
		for (size_t i = 0; i < l0.size() && i < l3.size(); i++) {
			e0 += double(l0[i]) * l0[i];
			e3 += double(l3[i]) * l3[i];
			diff += double(l3[i] - l0[i]) * (l3[i] - l0[i]);
		}
		const double db = 10.0 * std::log10(e3 / e0), ddb = 10.0 * std::log10(diff / e0);
		std::snprintf(buf, sizeof(buf), "level %+.2f dB, difference %.1f dB re. dry", db, ddb);
		check("chorus: different sound, similar level", std::fabs(db) < 3.0 && ddb > -30.0, buf);

		const std::string t = paramDisplay::value(DLY_TIME, 7.0 / 11.0, paramDisplay::Context()),
		                  m = paramDisplay::value(DLY_MIX, 0.0, paramDisplay::Context());
		std::snprintf(buf, sizeof(buf), "'%s' '%s'", t.c_str(), m.c_str());
		check("readout", t == "1/8." && m == "OFF", buf);
	}

	// ---- B-62: GUI edits reach the host (performEdit -> IComponentHandler) (2026-11-15) ----
	{
		std::printf("\nB-62 GUI edits reach the host\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		struct Handler : public IComponentHandler {
			int begins = 0, edits = 0, ends = 0;
			ParamID lastId = 0;
			ParamValue lastValue = -1;
			tresult PLUGIN_API beginEdit(ParamID) override { begins++; return kResultOk; }
			tresult PLUGIN_API performEdit(ParamID id, ParamValue v) override { edits++; lastId = id; lastValue = v; return kResultOk; }
			tresult PLUGIN_API endEdit(ParamID) override { ends++; return kResultOk; }
			tresult PLUGIN_API restartComponent(int32) override { return kResultOk; }
			tresult PLUGIN_API queryInterface(const TUID, void** obj) override { *obj = nullptr; return kNoInterface; }
			uint32 PLUGIN_API addRef() override { return 1; }
			uint32 PLUGIN_API release() override { return 1; }
		} handler;
		char buf[200];
		static HostApplication shost62;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&shost62);
		ctrl->setComponentHandler(&handler);
		ctrl->beginEdit(FILTER_);
		const tresult r = ctrl->performEdit(FILTER_, 0.3);
		ctrl->endEdit(FILTER_);
		std::snprintf(buf, sizeof(buf), "result %d, begin/edit/end %d/%d/%d, id %u = %.2f", int(r), handler.begins,
		              handler.edits, handler.ends, unsigned(handler.lastId), handler.lastValue);
		check("performEdit is passed to the host", r == kResultOk && handler.begins == 1 && handler.edits == 1 &&
		          handler.ends == 1 && handler.lastId == FILTER_ && std::fabs(handler.lastValue - 0.3) < 1e-9, buf);
		ctrl->setComponentHandler(nullptr);
		ctrl->terminate();
		ctrl->release();
	}

	// ---- B-57: the host's cutoff text uses the engine's curve (2026-11-15) ----
	{
		std::printf("\nB-57 cutoff scale (host text vs engine)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		static HostApplication shost57;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&shost57);
		Parameter* fp = ctrl->getParameterObject(FILTER_);
		check("FILTER_ parameter exists", fp != nullptr, "");
		if (fp) {
			const double vs[] = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0};
			for (double v : vs) {
				const double host = fp->toPlain(v), eng = paramDisplay::filterCutoffHz(v);
				std::snprintf(buf, sizeof(buf), "host %.1f Hz, engine %.1f Hz", host, eng);
				char nm[64];
				std::snprintf(nm, sizeof(nm), "cutoff at %.2f matches", v);
				check(nm, std::fabs(host - eng) <= eng * 0.001, buf);
			}
			const double def = fp->getInfo().defaultNormalizedValue;
			std::snprintf(buf, sizeof(buf), "default %.4f -> engine %.0f Hz", def, paramDisplay::filterCutoffHz(def));
			check("default is 10 kHz in the engine", std::fabs(paramDisplay::filterCutoffHz(def) - 10000.0) < 10.0, buf);
			const double n1k = fp->toNormalized(1000.0);
			std::snprintf(buf, sizeof(buf), "1000 Hz -> %.4f -> engine %.1f Hz", n1k, paramDisplay::filterCutoffHz(n1k));
			check("typed 1000 Hz plays 1000 Hz", std::fabs(paramDisplay::filterCutoffHz(n1k) - 1000.0) < 1.0, buf);
		}
		ctrl->terminate();
		ctrl->release();
	}

	// ---- Scope info: voice count and output peak (2026-11-14) ----
	{
		std::printf("\nScope info (voice count, output peak)\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		resetMsgCounts();
		g_peakMsgMax = 0.0;
		g_peakMsgZeroRun = g_peakMsgZeroRunMax = 0;
		std::vector<float> L, R;
		Options o;
		o.captureL = &L;
		o.captureR = &R;
		o.onCreated = [](rs::DX10Processor& p) { p.scopeEnabled.store(true); };
		o.inspect = [](rs::DX10Processor& p) { p.onTimer(nullptr); };  // send what the last blocks left
		render(Settings{"peak", 0.995, 0.85}, {{0, true, 48}, {0, true, 55}, {static_cast<int64>(0.2 * kSampleRate), false, 48},
		                                       {static_cast<int64>(0.2 * kSampleRate), false, 55}}, 0.6, o);
		float truePeak = 0.0f;
		for (size_t i = 0; i < L.size(); i++) truePeak = (std::max)(truePeak, (std::max)(std::fabs(L[i]), std::fabs(R[i])));
		std::snprintf(buf, sizeof(buf), "sent max %.6f, output %.6f", g_peakMsgMax, truePeak);
		check("peak messages carry the output's peak", truePeak > 0.01f && std::fabs(g_peakMsgMax - truePeak) < 1e-6, buf);
		const int byTimer = g_msgsOut[static_cast<unsigned char>(PMSG_PEAK_CHAR)];
		const int inProc = g_msgsIn[static_cast<unsigned char>(PMSG_PEAK_CHAR)];
		std::snprintf(buf, sizeof(buf), "%d sent by timer, %d inside process", byTimer, inProc);
		check("peak messages from the timer only", byTimer > 0 && inProc == 0, buf);
		std::snprintf(buf, sizeof(buf), "longest run of 0 messages: %d", g_peakMsgZeroRunMax);
		check("silence: one 0, then nothing", g_peakMsgZeroRunMax == 1, buf);

		static HostApplication shost5;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&shost5);
		HostMessage* m = new HostMessage;
		m->setMessageID("g");
		m->getAttributes()->setFloat("peak", 0.5);
		ctrl->notify(m);
		m->release();
		std::snprintf(buf, sizeof(buf), "%.3f, %u message(s)", ctrl->outPeak, ctrl->peakFrames);
		check("controller receives the peak", ctrl->outPeak == 0.5f && ctrl->peakFrames == 1, buf);
		ctrl->terminate();
		ctrl->release();

		unsigned char st[128] = {};
		st[40] = 0x80 | 90; st[52] = 0x80 | 3; st[60] = 50; st[61] = 1;
		int total = -1, held = -1;
		paramDisplay::countVoices(st, total, held);
		std::snprintf(buf, sizeof(buf), "%d voices, %d held", total, held);
		check("countVoices", total == 4 && held == 2, buf);
		const std::string t0 = paramDisplay::peakText(0.0f), t1 = paramDisplay::peakText(0.5f),
		                  t2 = paramDisplay::peakText(1.0f), t3 = paramDisplay::peakText(1.2f);
		std::snprintf(buf, sizeof(buf), "%s | %s | %s | %s", t0.c_str(), t1.c_str(), t2.c_str(), t3.c_str());
		check("peakText", t0 == "-inf dB" && t1 == "-6.0 dB" && t2 == "0.0 dB" && t3 == "+1.6 dB", buf);
	}

	// ---- Preset bar: state blob, processor messages, controller loading, randomizer (2026-11-14) ----
	{
		std::printf("\nPreset bar\n");
		auto check = [&](const char* name, bool ok, const char* detail) {
			if (!ok) failures++;
			std::printf("  %-52s %-48s %s\n", name, detail, ok ? "PASS" : "FAIL");
		};
		char buf[200];
		rs::DX10Processor::ensureFactoryPresets();
		auto factoryState = [](int k) {
			presetTools::State s;
			s.program = uint32(k);
			s.values.assign(GPARAMS, 0.0);
			for (int i = 0; i < GPARAMS; i++) s.values[i] = rs::DX10Processor::programParams[k][i];
			s.hasMasterVol = true;
			s.masterVol = DEFAULT_MASTER_VOL;
			return s;
		};

		// processor: getState -> parse -> write gives the same bytes; 'i' answers with that state;
		// 'h' loads a state
		bool roundTrip = false, replyOk = false, loadOk = false;
		std::string detail;
		Options o;
		o.inspect = [&](rs::DX10Processor& p) {
			MemoryStream ms;
			p.getState(&ms);
			const std::vector<char> orig(ms.getData(), ms.getData() + ms.getSize());
			presetTools::State s;
			roundTrip = presetTools::parseState(orig.data(), orig.size(), s) && presetTools::writeState(s) == orig;

			g_stateMsg.clear();
			HostMessage* q = new HostMessage;
			q->setMessageID("i");
			q->getAttributes()->setInt("purpose", 7);
			p.notify(q);
			q->release();
			replyOk = g_stateMsg == orig && g_stateMsgPurpose == 7;

			s.values[FILTER_] = 0.123;
			s.values[GAIN_] = 0.654;
			const std::vector<char> blob = presetTools::writeState(s);
			HostMessage* h = new HostMessage;
			h->setMessageID("h");
			h->getAttributes()->setBinary("data", blob.data(), uint32(blob.size()));
			p.notify(h);
			h->release();
			loadOk = std::fabs(p.floatParams[FILTER_] - 0.123) < 1e-12 && std::fabs(p.floatParams[GAIN_] - 0.654) < 1e-12;
		};
		render(Settings{"pstate", 0.995, 0.85}, {}, 0.05, o);
		check("state parse/write round trip", roundTrip, roundTrip ? "identical bytes" : "differs");
		check("get-state message answers with the state", replyOk, replyOk ? "same bytes, purpose 7" : "missing/differs");
		check("set-state message loads the state", loadOk, loadOk ? "FILTER_ 0.123, GAIN_ 0.654" : "not applied");

		// controller: factory preset, master volume kept, name
		static HostApplication shost6;
		auto* ctrl = new rs::DX10Controller();
		ctrl->initialize(&shost6);
		ctrl->setParamNormalized(MASTER_VOL, 0.3);
		ctrl->loadFactoryPreset(2);
		const double cf = ctrl->getParamNormalized(FILTER_), pf = rs::DX10Processor::programParams[2][FILTER_];
		std::snprintf(buf, sizeof(buf), "%s, FILTER_ %.4f (preset %.4f), vol %.2f", ctrl->presetName.c_str(), cf, pf,
		              ctrl->getParamNormalized(MASTER_VOL));
		check("factory preset loads, volume kept", ctrl->presetName == "Perturb" && std::fabs(cf - pf) < 1e-6 &&
		          std::fabs(ctrl->getParamNormalized(MASTER_VOL) - 0.3) < 1e-9 && ctrl->presetFactory == 2, buf);

		// a .vstpreset file (written like Save does) loads by name
		wchar_t tmp[MAX_PATH] = {};
		GetTempPathW(MAX_PATH, tmp);
		const std::wstring dir = std::wstring(tmp) + L"dm_preset_test";
		const std::wstring path = dir + L"\\Test Bass.vstpreset";
		presetTools::State fs = factoryState(4);
		fs.values[FILTER_] = 0.321;
		std::vector<char> comp = presetTools::writeState(fs);
		MemoryStream compStream(comp.data(), TSize(comp.size()));
		MemoryStream out;
		const bool saved = PresetFile::savePreset(&out, rs::DX10Processor::uid, &compStream) &&
		                   presetTools::writeFile(path, std::vector<char>(out.getData(), out.getData() + out.getSize()));
		const bool loaded = saved && ctrl->loadPresetFile(path);
		std::snprintf(buf, sizeof(buf), "%s, FILTER_ %.3f, vol %.2f", ctrl->presetName.c_str(), ctrl->getParamNormalized(FILTER_),
		              ctrl->getParamNormalized(MASTER_VOL));
		check("preset file loads (name, values, volume kept)", loaded && ctrl->presetName == "Test Bass" &&
		          std::fabs(ctrl->getParamNormalized(FILTER_) - 0.321) < 1e-6 && ctrl->presetFactory == -1 &&
		          std::fabs(ctrl->getParamNormalized(MASTER_VOL) - 0.3) < 1e-9, buf);
		DeleteFileW(path.c_str());
		RemoveDirectoryW(dir.c_str());

		// controller state keeps the preset bar's name
		ctrl->presetDirty = true;
		MemoryStream cs;
		ctrl->getState(&cs);
		cs.seek(0, IBStream::kIBSeekSet, nullptr);
		auto* ctrl2 = new rs::DX10Controller();
		ctrl2->initialize(&shost6);
		ctrl2->setState(&cs);
		std::snprintf(buf, sizeof(buf), "%s%s, factory %d", ctrl2->presetName.c_str(), ctrl2->presetDirty ? " *" : "", ctrl2->presetFactory);
		check("controller state keeps the preset name", ctrl2->presetName == "Test Bass" && ctrl2->presetDirty &&
		          ctrl2->presetPath == path && ctrl2->presetFactory == -1, buf);
		ctrl2->terminate();
		ctrl2->release();

		// randomizer: what changes
		std::vector<int> steps;
		ctrl->presetStepCounts(steps);
		const int kept[] = {TUNE_, FINE_, OCT_, INT_, MASTER_VOL, STEP_CTRL_TARG, ENV_1_DELAY};
		{
			const presetTools::State a = factoryState(1);
			presetTools::State b = a;
			presetTools::randomize(b, steps.data(), steps.size(), presetTools::Mode::kVariation, 7);
			int changed = 0, steppedChanged = 0, tooFar = 0, keptChanged = 0;
			for (int i = 0; i < GPARAMS; i++) {
				if (b.values[i] == a.values[i]) continue;
				changed++;
				if (steps[i] > 0) steppedChanged++;
				if (std::fabs(b.values[i] - a.values[i]) > 0.1 + 1e-9) tooFar++;
			}
			for (int k : kept) if (b.values[k] != a.values[k]) keptChanged++;
			std::snprintf(buf, sizeof(buf), "%d changed, %d stepped, %d > 0.1, %d kept ids", changed, steppedChanged, tooFar, keptChanged);
			check("variation: small moves of continuous values", changed >= 8 && steppedChanged == 0 && tooFar == 0 && keptChanged == 0, buf);
		}
		{
			const presetTools::State a = factoryState(0);
			presetTools::State b = a;
			presetTools::randomize(b, steps.data(), steps.size(), presetTools::Mode::kFull, 11);
			int changed = 0, keptChanged = 0, outside = 0;
			for (int i = 0; i < GPARAMS; i++) {
				if (b.values[i] != a.values[i]) changed++;
				if (b.values[i] < 0.0 || b.values[i] > 1.0) outside++;
			}
			for (int k : kept) if (b.values[k] != a.values[k]) keptChanged++;
			std::snprintf(buf, sizeof(buf), "%d changed, %d kept ids moved, %d outside 0..1", changed, keptChanged, outside);
			check("full random: new values, kept ids stay", changed >= 30 && keptChanged == 0 && outside == 0, buf);
		}
		ctrl->terminate();
		ctrl->release();

		// full random patches must sound: 24 seeds, a held chord, RMS over 0.15..0.6 s
		int quiet = 0, bad = 0;
		float minRms = 1e9f, maxPeak = 0.0f;
		int minSeed = -1;
		for (int seed = 1; seed <= 24; seed++) {
			presetTools::State s = factoryState(seed % rs::DX10Processor::kNumPrograms);
			presetTools::randomize(s, steps.data(), steps.size(), presetTools::Mode::kFull, uint32_t(seed));
			const std::vector<char> blob = presetTools::writeState(s);
			std::vector<float> L, R;
			Options ro;
			ro.keepPatchEnvelopes = true;
			ro.captureL = &L;
			ro.captureR = &R;
			ro.onCreated = [&](rs::DX10Processor& p) {
				MemoryStream ms(const_cast<char*>(blob.data()), TSize(blob.size()));
				static_cast<IComponent&>(p).setState(&ms);  // public through the interface
			};
			render(Settings{"rnd", 0.995, 0.85}, {{0, true, 48}, {0, true, 55}, {0, true, 64}}, 0.6, ro);
			double sum = 0.0;
			float pk = 0.0f;
			size_t n = 0;
			bool nan = false;
			for (size_t i = size_t(0.15 * kSampleRate); i < L.size(); i++) {
				if (!std::isfinite(L[i]) || !std::isfinite(R[i])) nan = true;
				sum += double(L[i]) * L[i] + double(R[i]) * R[i];
				pk = (std::max)(pk, (std::max)(std::fabs(L[i]), std::fabs(R[i])));
				n += 2;
			}
			const float rms = n ? float(std::sqrt(sum / double(n))) : 0.0f;
			if (rms < 0.003f) quiet++;
			if (nan || pk > 4.0f) bad++;
			if (rms < minRms) { minRms = rms; minSeed = seed; }
			maxPeak = (std::max)(maxPeak, pk);
		}
		std::snprintf(buf, sizeof(buf), "%d quiet, %d bad; min RMS %.4f (seed %d), max peak %.2f", quiet, bad, minRms, minSeed, maxPeak);
		check("24 full random patches all sound", quiet == 0 && bad == 0, buf);

		if (std::getenv("DM_RND_DIAG")) {  // which parameter silences a quiet random patch
			auto rmsOf = [&](const presetTools::State& st) {
				const std::vector<char> blob = presetTools::writeState(st);
				std::vector<float> L, R;
				Options ro;
				ro.keepPatchEnvelopes = true;
				ro.captureL = &L;
				ro.captureR = &R;
				ro.onCreated = [&](rs::DX10Processor& p) {
					MemoryStream ms(const_cast<char*>(blob.data()), TSize(blob.size()));
					static_cast<IComponent&>(p).setState(&ms);
				};
				render(Settings{"rnd", 0.995, 0.85}, {{0, true, 48}, {0, true, 55}, {0, true, 64}}, 0.6, ro);
				double sum = 0.0;
				size_t n = 0;
				for (size_t i = size_t(0.15 * kSampleRate); i < L.size(); i++) {
					sum += double(L[i]) * L[i] + double(R[i]) * R[i];
					n += 2;
				}
				return n ? std::sqrt(sum / double(n)) : 0.0;
			};
			for (int seed = 1; seed <= 24; seed++) {
				const presetTools::State a = factoryState(seed % rs::DX10Processor::kNumPrograms);
				presetTools::State b = a;
				presetTools::randomize(b, steps.data(), steps.size(), presetTools::Mode::kFull, uint32_t(seed));
				const double r = rmsOf(b);
				if (r >= 0.003)
					continue;
				std::printf("    seed %d quiet (%.5f), factory %.4f; reverting one id:", seed, r, rmsOf(a));
				for (int i = 0; i < GPARAMS; i++) {
					if (a.values[i] == b.values[i]) continue;
					presetTools::State c = b;
					c.values[i] = a.values[i];
					const double rc = rmsOf(c);
					if (rc >= 0.003)
						std::printf(" %d(%.3f->%.3f)=%.4f", i, b.values[i], a.values[i], rc);
				}
				std::printf("\n");
			}
		}
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
