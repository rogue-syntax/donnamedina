// film.exe (2026-11-21): renders Donna Medina from a MIDI file into material for video: the editor
// as PNG layers (transparent, at 1..4x the editor size), the synth's sound as a WAV, and a data
// file with what the engine and the editor do in every frame. Never shipped. See docs/08-video.md.
//
//   film.exe song.mid [--out DIR] [--fps 30] [--scale 2] [--preset NAME | --preset-file FILE.vstpreset]
//            [--track N] [--channel N] [--start SEC] [--end SEC] [--tail SEC] [--sr 48000]
//            [--layers base,displays,art,controls,overlays | all] [--automate ID:T0:V0:T1:V1]...
//   film.exe --record take.rsrec [same options but --preset*, --track, --channel]
//     replays a take from a recording build of the plugin (source/videoRecord.h): the state, notes,
//     automation and transport as the host sent them during an export.
//
// Everything is driven by the frame clock: audio is rendered up to each frame's sample position,
// then the processor's message timer runs once (scope, voices, playheads, peak go to the editor),
// the editor's clock (uiClock) is set to the frame's time, and each layer is drawn into an
// off-screen framebuffer. The window's message loop does not run while frames are made, so nothing
// draws on wall-clock time: the same input gives the same files.

#include <windows.h>
#include <wincodec.h>
#include "../../source/mdaDX10Processor.h"
#include "../../source/mdaBaseController.h"
#include "../../source/mdaDX10Controller.h"
#include "../../source/NVGView.h"
#include "../../source/presetTools.h"
#include "../../source/factoryPresetFiles.h"
#include "../../source/uiClock.h"
#include "../../source/updateCheck.h"
#include "../../source/license.h"
#include "../../source/editorLayout.h"
#include "../../source/videoRecord.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/gui/iplugview.h"
#include "midiFile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VSTGUI;

void* moduleHandle = nullptr;  // VSTGUI's resource module (the plugin's .res is linked into this exe)

namespace {

// ---- options ----------------------------------------------------------------------------------

struct Automation {
	ParamID id = 0;
	double t0 = 0, v0 = 0, t1 = 0, v1 = 0;
	double at(double t) const {
		if (t <= t0) return v0;
		if (t >= t1) return v1;
		return v0 + (v1 - v0) * (t - t0) / (t1 - t0);
	}
};

struct LayerDef {
	const char* name;
	unsigned bits;
};
const LayerDef kLayers[] = {
	{"base", rs::NVGView::kLayerBase},
	{"displays", rs::NVGView::kLayerDisplays},
	{"art", rs::NVGView::kLayerArt},
	{"controls", rs::NVGView::kLayerControls},
	{"overlays", rs::NVGView::kLayerOverlays},
};

struct Options {
	std::wstring midi, record, out, presetFile;
	bool offline = false;  // --offline: process in offline mode (a recording build then records a take)
	int block = 256;       // --block: MIDI block size (a take keeps the host's)
	std::string presetName;
	int fps = 30;
	double scale = 2.0;
	int track = -1, channel = -1;
	double start = 0.0, end = -1.0, tail = 1.0;
	double sampleRate = 48000.0;
	std::vector<LayerDef> layers{std::begin(kLayers), std::end(kLayers)};
	std::vector<Automation> automation;
};

std::wstring widen(const std::string& s) { return presetTools::fromUtf8(s); }
std::string narrow(const std::wstring& s) { return presetTools::toUtf8(s); }

void usage() {
	std::printf(
	    "film.exe song.mid [options]       or   film.exe --record take.rsrec [options]\n"
	    "  --record FILE         replay a take from a recording build (notes, automation, tempo and the\n"
	    "                        sound as the host sent them); --start/--end are then take seconds\n"
	    "  --out DIR             output folder (default video\\out\\film\\<input name>)\n"
	    "  --fps N               frames per second (default 30)\n"
	    "  --scale S             editor scale, 1..4 (default 2: 2496 x 1084)\n"
	    "  --preset NAME         a factory preset (default the first)\n"
	    "  --preset-file FILE    a .vstpreset file\n"
	    "  --track N, --channel N  only this track (0 = first) / MIDI channel (1..16)\n"
	    "  --start SEC, --end SEC  the part to film (default: the whole file plus --tail)\n"
	    "  --tail SEC            after the last MIDI event (default 1)\n"
	    "  --sr HZ               sample rate (default 48000)\n"
	    "  --layers LIST|all     base,displays,art,controls,overlays (default all five, one folder each);\n"
	    "                        'all' = one folder with the whole editor in one pass\n"
	    "  --automate ID:T0:V0:T1:V1  ramp parameter ID (normalized V0 -> V1, seconds T0 -> T1); repeatable\n"
	    "  --offline             tell the processor it renders offline (tests the recorder of a recording build)\n"
	    "  --block N             MIDI: samples per process() call (default 256; a frame also ends a block)\n");
}

bool parseArgs(int argc, wchar_t** argv, Options& o) {
	for (int i = 1; i < argc; i++) {
		const std::wstring a = argv[i];
		auto next = [&]() -> std::wstring { return i + 1 < argc ? std::wstring(argv[++i]) : std::wstring(); };
		if (a == L"--out") o.out = next();
		else if (a == L"--fps") o.fps = _wtoi(next().c_str());
		else if (a == L"--scale") o.scale = _wtof(next().c_str());
		else if (a == L"--preset") o.presetName = narrow(next());
		else if (a == L"--preset-file") o.presetFile = next();
		else if (a == L"--track") o.track = _wtoi(next().c_str());
		else if (a == L"--channel") o.channel = _wtoi(next().c_str()) - 1;
		else if (a == L"--start") o.start = _wtof(next().c_str());
		else if (a == L"--end") o.end = _wtof(next().c_str());
		else if (a == L"--tail") o.tail = _wtof(next().c_str());
		else if (a == L"--sr") o.sampleRate = _wtof(next().c_str());
		else if (a == L"--layers") {
			const std::string list = narrow(next());
			o.layers.clear();
			if (list == "all") {
				o.layers.push_back({"all", rs::NVGView::kLayerBase | rs::NVGView::kLayerDisplays | rs::NVGView::kLayerArt |
				                               rs::NVGView::kLayerControls | rs::NVGView::kLayerOverlays});
			} else {
				size_t p = 0;
				while (p <= list.size()) {
					const size_t q = std::min(list.find(',', p), list.size());
					const std::string n = list.substr(p, q - p);
					bool found = false;
					for (const LayerDef& d : kLayers)
						if (n == d.name) { o.layers.push_back(d); found = true; }
					if (!found) { std::printf("unknown layer '%s'\n", n.c_str()); return false; }
					p = q + 1;
				}
			}
		} else if (a == L"--automate") {
			Automation x;
			unsigned id = 0;
			if (swscanf(next().c_str(), L"%u:%lf:%lf:%lf:%lf", &id, &x.t0, &x.v0, &x.t1, &x.v1) != 5 || x.t1 <= x.t0) {
				std::printf("--automate wants ID:T0:V0:T1:V1 with T1 > T0\n");
				return false;
			}
			x.id = id;
			o.automation.push_back(x);
		} else if (a == L"--record") o.record = next();
		else if (a == L"--offline") o.offline = true;
		else if (a == L"--block") o.block = std::max(16, std::min(8192, _wtoi(next().c_str())));
		else if (a == L"--help" || a == L"-h") return false;
		else if (!a.empty() && a[0] != L'-' && o.midi.empty()) o.midi = a;
		else { std::printf("unknown option %s\n", narrow(a).c_str()); return false; }
	}
	if (o.midi.empty() == o.record.empty()) {
		if (!o.midi.empty()) std::printf("give a MIDI file or --record, not both\n");
		return false;
	}
	if (!o.record.empty() && (!o.presetName.empty() || !o.presetFile.empty() || o.track >= 0 || o.channel >= 0)) {
		std::printf("--record replays the recorded sound and notes: no --preset, --preset-file, --track, --channel\n");
		return false;
	}
	if (o.fps < 1 || o.fps > 240) { std::printf("--fps 1..240\n"); return false; }
	if (!(o.scale >= 1.0 && o.scale <= 4.0)) { std::printf("--scale 1..4\n"); return false; }
	if (!(o.sampleRate >= 22050.0 && o.sampleRate <= 192000.0)) { std::printf("--sr 22050..192000\n"); return false; }
	return true;
}

// ---- a recorded take (source/videoRecord.h) -----------------------------------------------------

struct RecBlock {
	videorec::BlockHeader h{};
	std::vector<videorec::ParamPoint> params;
	std::vector<videorec::NoteEvent> events;
	int64 at = 0;  // take sample where the block starts
};
struct Take {
	bool ok = false;
	std::string error, writer;
	videorec::FileHeader h{};
	std::vector<uint8_t> state;
	std::vector<RecBlock> blocks;
	int64 length = 0;   // samples
	int32 maxBlock = 0;  // longest block
};

Take readTake(const std::wstring& path) {
	Take t;
	FILE* f = _wfopen(path.c_str(), L"rb");
	if (!f) { t.error = "cannot open"; return t; }
	std::vector<uint8_t> d;
	uint8_t chunk[65536];
	for (size_t n; (n = std::fread(chunk, 1, sizeof(chunk), f)) > 0;) d.insert(d.end(), chunk, chunk + n);
	std::fclose(f);
	size_t p = 0;
	auto take = [&](void* dst, size_t n) { if (p + n > d.size()) return false; std::memcpy(dst, d.data() + p, n); p += n; return true; };
	char writer[32] = {};
	if (!take(&t.h, sizeof(t.h)) || std::memcmp(t.h.magic, videorec::kMagic, sizeof(videorec::kMagic)) != 0) { t.error = "not a take file"; return t; }
	if (t.h.version != videorec::kVersion) { t.error = "take version " + std::to_string(t.h.version) + ", this tool reads " + std::to_string(videorec::kVersion); return t; }
	if (!take(writer, sizeof(writer))) { t.error = "truncated header"; return t; }
	writer[31] = 0;
	t.writer = writer;
	t.state.resize(t.h.stateSize);
	if (t.h.stateSize && !take(t.state.data(), t.h.stateSize)) { t.error = "truncated state"; return t; }
	while (p < d.size()) {
		RecBlock b;
		if (!take(&b.h, sizeof(b.h)) || b.h.numParams < 0 || b.h.numEvents < 0 || b.h.numSamples < 0) { t.error = "truncated block"; return t; }
		b.params.resize(size_t(b.h.numParams));
		b.events.resize(size_t(b.h.numEvents));
		if ((b.h.numParams && !take(b.params.data(), b.params.size() * sizeof(videorec::ParamPoint))) ||
		    (b.h.numEvents && !take(b.events.data(), b.events.size() * sizeof(videorec::NoteEvent)))) { t.error = "truncated block"; return t; }
		b.at = t.length;
		t.length += b.h.numSamples;
		t.maxBlock = std::max<int32>(t.maxBlock, b.h.numSamples);
		t.blocks.push_back(std::move(b));
	}
	t.ok = true;
	return t;
}

// the recorded block that holds take sample s
const RecBlock* blockAt(const Take& t, int64 s) {
	if (t.blocks.empty()) return nullptr;
	auto it = std::upper_bound(t.blocks.begin(), t.blocks.end(), s, [](int64 v, const RecBlock& b) { return v < b.at; });
	return it == t.blocks.begin() ? &t.blocks.front() : &*(it - 1);
}

// ---- files ------------------------------------------------------------------------------------

IWICImagingFactory* g_wic = nullptr;

// bgra: top-down, straight (not premultiplied) alpha
bool savePng(const std::wstring& path, int w, int h, const uint8_t* bgra) {
	IWICStream* stream = nullptr;
	IWICBitmapEncoder* enc = nullptr;
	IWICBitmapFrameEncode* frame = nullptr;
	bool ok = SUCCEEDED(g_wic->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
	          SUCCEEDED(g_wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc)) &&
	          SUCCEEDED(enc->Initialize(stream, WICBitmapEncoderNoCache)) && SUCCEEDED(enc->CreateNewFrame(&frame, nullptr)) &&
	          SUCCEEDED(frame->Initialize(nullptr)) && SUCCEEDED(frame->SetSize(UINT(w), UINT(h)));
	if (ok) {
		WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
		ok = SUCCEEDED(frame->SetPixelFormat(&fmt)) && IsEqualGUID(fmt, GUID_WICPixelFormat32bppBGRA) &&
		     SUCCEEDED(frame->WritePixels(UINT(h), UINT(w * 4), UINT(w * 4 * h), const_cast<BYTE*>(bgra))) &&
		     SUCCEEDED(frame->Commit()) && SUCCEEDED(enc->Commit());
	}
	if (frame) frame->Release();
	if (enc) enc->Release();
	if (stream) stream->Release();
	return ok;
}

bool saveWav(const std::wstring& path, const std::vector<float>& l, const std::vector<float>& r, int sr) {
	FILE* f = _wfopen(path.c_str(), L"wb");
	if (!f) return false;
	const uint32_t n = uint32_t(l.size()), dataBytes = n * 8;
	auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
	auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
	std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); std::fwrite("WAVE", 1, 4, f);
	std::fwrite("fmt ", 1, 4, f); u32(16); u16(3); u16(2); u32(uint32_t(sr)); u32(uint32_t(sr) * 8); u16(8); u16(32);  // float32 stereo
	std::fwrite("data", 1, 4, f); u32(dataBytes);
	for (uint32_t i = 0; i < n; i++) { std::fwrite(&l[i], 4, 1, f); std::fwrite(&r[i], 4, 1, f); }
	std::fclose(f);
	return true;
}

std::string jsonStr(const std::string& s) {
	std::string o = "\"";
	for (char c : s) {
		if (c == '"' || c == '\\') { o += '\\'; o += c; }
		else if (static_cast<unsigned char>(c) < 0x20) { char b[8]; std::snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
		else o += c;
	}
	return o + "\"";
}

void pump(int ms) {
	const ULONGLONG end = GetTickCount64() + ms;
	do {
		MSG m;
		while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessage(&m); }
		Sleep(5);
	} while (GetTickCount64() < end);
}

// ---- the data file ----------------------------------------------------------------------------

// One value per frame for each channel; channels that never change go to "constant".
struct DataTable {
	std::vector<std::string> order;
	std::map<std::string, std::vector<float>> ch;
	void set(int frame, const std::string& name, float v) {
		auto it = ch.find(name);
		if (it == ch.end()) { order.push_back(name); it = ch.emplace(name, std::vector<float>()).first; }
		it->second.resize(size_t(frame) + 1, 0.0f);
		it->second[size_t(frame)] = v;
	}
	bool write(const std::wstring& path, int frames, const std::string& header) const {
		FILE* f = _wfopen(path.c_str(), L"wb");
		if (!f) return false;
		std::fprintf(f, "{\n%s,\n  \"channels\": {", header.c_str());
		bool first = true;
		std::string constants;
		for (const std::string& n : order) {
			std::vector<float> v = ch.at(n);
			v.resize(size_t(frames), v.empty() ? 0.0f : v.back());
			bool same = true;
			for (float x : v) if (x != v[0]) { same = false; break; }
			char b[48];
			if (same) {
				std::snprintf(b, sizeof(b), "%.6g", v.empty() ? 0.0 : double(v[0]));
				constants += std::string(constants.empty() ? "\n    " : ",\n    ") + jsonStr(n) + ": " + b;
				continue;
			}
			std::fprintf(f, "%s\n    %s: [", first ? "" : ",", jsonStr(n).c_str());
			first = false;
			for (size_t i = 0; i < v.size(); i++) {
				std::snprintf(b, sizeof(b), "%s%.6g", i ? "," : "", double(v[i]));
				std::fputs(b, f);
			}
			std::fputs("]", f);
		}
		std::fprintf(f, "\n  },\n  \"constant\": {%s\n  }\n}\n", constants.c_str());
		std::fclose(f);
		return true;
	}
};

const char* const kSrcNames[6] = {"env1", "env2", "env3", "lfo1", "lfo2", "lfo3"};

} // namespace

int wmain(int argc, wchar_t** argv) {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	Options opt;
	if (!parseArgs(argc, argv, opt)) { usage(); return 1; }

	// ---- the input: a MIDI file, or a take recorded by a recording build ----
	const bool rec = !opt.record.empty();
	const std::wstring input = rec ? opt.record : opt.midi;
	midifile::File midi;
	Take take;
	std::vector<midifile::Event> events;
	std::vector<int> pitchUsed(128, 0);
	double endSec = 0.0;
	if (rec) {
		take = readTake(opt.record);
		if (!take.ok) { std::printf("%s: %s\n", narrow(opt.record).c_str(), take.error.c_str()); return 1; }
		if (!(take.h.sampleRate >= 8000.0 && take.h.sampleRate <= 384000.0)) { std::printf("take sample rate %g?\n", take.h.sampleRate); return 1; }
		opt.sampleRate = take.h.sampleRate;
		size_t notes = 0, points = 0;
		for (const RecBlock& b : take.blocks) {
			points += b.params.size();
			for (const videorec::NoteEvent& e : b.events)
				if (e.type == Event::kNoteOnEvent) { pitchUsed[e.pitch & 127] = 1; notes++; }
		}
		const double len = double(take.length) / opt.sampleRate;
		endSec = opt.end > 0.0 ? std::min(opt.end, len) : len;  // the export already has its tail
		std::printf("Take: %.3f s at %g Hz in %zu blocks (up to %d samples), %zu notes, %zu parameter points; filming %.3f .. %.3f s\n",
		            len, opt.sampleRate, take.blocks.size(), take.maxBlock, notes, points, opt.start, endSec);
	} else {
		midi = midifile::read(opt.midi);
		if (!midi.ok) { std::printf("%s: %s\n", narrow(opt.midi).c_str(), midi.error.c_str()); return 1; }
		for (const midifile::Event& e : midi.events) {
			if (opt.track >= 0 && e.track != opt.track) continue;
			if (opt.channel >= 0 && e.channel() != opt.channel) continue;
			events.push_back(e);
			if (e.isNoteOn()) pitchUsed[e.d1] = 1;
		}
		double lastSec = 0.0;
		for (const midifile::Event& e : events) lastSec = std::max(lastSec, e.sec);
		endSec = opt.end > 0.0 ? opt.end : lastSec + opt.tail;
		std::printf("MIDI: %zu events (of %zu), %d track(s), %.1f BPM at the start; filming %.3f .. %.3f s\n", events.size(),
		            midi.events.size(), midi.tracks, midi.bpmAtSec(0.0), opt.start, endSec);
	}
	if (endSec <= opt.start) { std::printf("nothing to film: end %.3f s <= start %.3f s\n", endSec, opt.start); return 1; }
	// the song at timeline time t (MIDI: song seconds; take: seconds from the start of the take)
	auto songSeconds = [&](double t) {
		if (!rec) return t;
		const RecBlock* b = blockAt(take, int64(std::llround(t * opt.sampleRate)));
		return b ? (double(b->h.projectTimeSamples) + (t * opt.sampleRate - double(b->at))) / opt.sampleRate : t;
	};
	auto songBpm = [&](double t) {
		if (!rec) return midi.bpmAtSec(t);
		const RecBlock* b = blockAt(take, int64(std::llround(t * opt.sampleRate)));
		return b && (b->h.ctxState & ProcessContext::kTempoValid) ? b->h.tempo : 0.0;
	};
	auto songQuarters = [&](double t) {
		if (!rec) return midi.quartersAtSec(t);
		const RecBlock* b = blockAt(take, int64(std::llround(t * opt.sampleRate)));
		if (!b || !(b->h.ctxState & ProcessContext::kProjectTimeMusicValid)) return 0.0;
		return b->h.projectTimeMusic + (t * opt.sampleRate - double(b->at)) / opt.sampleRate * b->h.tempo / 60.0;
	};

	// ---- the licence: the same check the plugin makes ----
	{
		license::License l;
		if (!license::checkStored(l)) {
			std::printf("No Donna Medina licence on this machine: the synth would stay silent. Paste your licence in the plugin first.\n");
			return 1;
		}
	}

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);  // VSTGUI decodes its PNGs through WIC; so do we
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_wic)))) {
		std::printf("WIC not available\n");
		return 1;
	}
	moduleHandle = GetModuleHandle(nullptr);
	updatecheck::suppressChecks(true);  // no network from the film tool
	uiClock::setManualSeconds(1000.0);  // the editor's clock stands still until the first frame

	// ---- output folders ----
	std::wstring out = opt.out;
	if (out.empty()) {
		std::wstring stem = input.substr(input.find_last_of(L"\\/") + 1);
		stem = stem.substr(0, stem.find_last_of(L'.'));
		out = L"video\\out\\film\\" + stem;
	}
	{  // absolute (presetTools::ensureDir walks up from a full path)
		wchar_t full[MAX_PATH * 2];
		if (GetFullPathNameW(out.c_str(), DWORD(std::size(full)), full, nullptr)) out = full;
	}
	presetTools::ensureDir(out);
	for (const LayerDef& d : opt.layers) presetTools::ensureDir(out + L"\\" + widen(d.name));

	// ---- processor and controller, connected as a host connects them ----
	static HostApplication host;
	auto* proc = new rs::DX10Processor();
	auto* ctrl = new rs::DX10Controller();
	if (proc->initialize(&host) != kResultTrue || ctrl->initialize(&host) != kResultTrue) { std::printf("initialize failed\n"); return 1; }
	static_cast<Vst::IConnectionPoint*>(proc)->connect(static_cast<Vst::IConnectionPoint*>(ctrl));
	static_cast<Vst::IConnectionPoint*>(ctrl)->connect(static_cast<Vst::IConnectionPoint*>(proc));
	const int32 kMaxBlock = opt.block;  // MIDI: the block size; a take keeps the host's blocks
	const int32 maxBlock = std::max<int32>(kMaxBlock, rec ? std::max<int32>(take.maxBlock, take.h.maxBlock) : 0);
	const ProcessModes mode = opt.offline ? kOffline : kRealtime;
	ProcessSetup setup{mode, kSample32, maxBlock, opt.sampleRate};
	proc->setupProcessing(setup);
	proc->setActive(true);

	// ---- the sound ----
	std::string presetLabel;
	if (rec) {  // the processor's state at the start of the export, as a host restores a project
		MemoryStream a(take.state.data(), TSize(take.state.size()));
		MemoryStream b(take.state.data(), TSize(take.state.size()));
		if (take.state.empty() || static_cast<IComponent*>(proc)->setState(&a) != kResultTrue ||
		    ctrl->setComponentState(&b) != kResultTrue) {
			std::printf("cannot restore the recorded state\n");
			return 1;
		}
		presetLabel = "recorded state";
	} else if (!opt.presetFile.empty()) {
		if (!ctrl->loadPresetFile(opt.presetFile)) { std::printf("cannot load %s\n", narrow(opt.presetFile).c_str()); return 1; }
		presetLabel = narrow(opt.presetFile);
	} else {
		int idx = 0;
		if (!opt.presetName.empty()) {
			idx = -1;
			for (int i = 0; i < rs::DX10Processor::kNumPrograms; i++)
				if (_stricmp(rs::DX10Controller::factoryPresetName(i), opt.presetName.c_str()) == 0) idx = i;
			if (idx < 0) {
				std::printf("no factory preset '%s'; there are:", opt.presetName.c_str());
				for (int i = 0; i < rs::DX10Processor::kNumPrograms; i++) std::printf(" \"%s\"", rs::DX10Controller::factoryPresetName(i));
				std::printf("\n");
				return 1;
			}
		}
		ctrl->loadFactoryPreset(idx);
		presetLabel = rs::DX10Controller::factoryPresetName(idx);
	}

	// ---- the editor, in a window off screen ----
	IPlugView* view = ctrl->createView(ViewType::kEditor);
	if (!view) { std::printf("createView failed\n"); return 1; }
	ViewRect vr;
	view->getSize(&vr);
	const int W = vr.getWidth(), H = vr.getHeight();
	RECT wr{0, 0, W, H};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwnd = CreateWindowExW(0, L"STATIC", L"Donna Medina film", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, -4000, 0,
	                            wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	ShowWindow(hwnd, SW_SHOWNOACTIVATE);
	if (view->attached(hwnd, kPlatformTypeHWND) != kResultTrue) { std::printf("attached failed\n"); return 1; }
	ViewRect tmp(0, 0, W - 1, H - 1);
	view->onSize(&tmp);
	view->onSize(&vr);
	pump(1200);  // the editor finishes opening (GL context, images, first frames, editor-open messages)
	rs::NVGView* nv = ctrl->nvgViewController ? ctrl->nvgViewController->nvgView : nullptr;
	if (!nv || !nv->vg || !nv->makeGLCurrent()) { std::printf("the editor's OpenGL view did not come up\n"); return 1; }
	nv->drawOpenGL(CRect());  // once by hand: first-frame work (view-ready catch-up)
	// the (modded) Win32OpenGLView::swapBuffers releases the context: take it again
	if (!nv->makeGLCurrent()) { std::printf("cannot make the editor's OpenGL context current\n"); return 1; }
	// The CONTROL readout reveals and fades a step per drawn frame: start it empty (as the editor's
	// first frame does), or its state at frame 0 depends on how often the editor drew while opening.
	nv->targDisplayTxt.clear();
	nv->targValueTxt.clear();
	std::srand(1);  // the readout's letter scramble uses rand()

	// ---- off-screen framebuffer at the chosen scale ----
	const int PW = int(std::lround(W * opt.scale)), PH = int(std::lround(H * opt.scale));
	GLuint fbo = 0, color = 0, depthStencil = 0;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenRenderbuffers(1, &color);
	glBindRenderbuffer(GL_RENDERBUFFER, color);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, PW, PH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
	glGenRenderbuffers(1, &depthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, PW, PH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
	const GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::printf("framebuffer %d x %d not available (status 0x%04X, GL error 0x%04X, OpenGL %s, %s)\n", PW, PH, fbStatus,
		            glGetError(), reinterpret_cast<const char*>(glGetString(GL_VERSION)),
		            reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		return 1;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	std::vector<uint8_t> pixels(size_t(PW) * PH * 4), image(size_t(PW) * PH * 4);

	auto drawLayer = [&](unsigned bits, const std::wstring& path) -> bool {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, PW, PH);
		glClearColor(0, 0, 0, 0);
		glStencilMask(0xff);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		nvgBeginFrame(nv->vg, float(W), float(H), float(opt.scale));
		try {
			nv->drawScene(bits);
		} catch (const std::exception&) {
			nvgCancelFrame(nv->vg);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}
		nvgEndFrame(nv->vg);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, PW, PH, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// bottom-up premultiplied (NanoVG) -> top-down straight alpha (PNG)
		for (int y = 0; y < PH; y++) {
			const uint8_t* s = &pixels[size_t(PH - 1 - y) * PW * 4];
			uint8_t* d = &image[size_t(y) * PW * 4];
			for (int x = 0; x < PW; x++, s += 4, d += 4) {
				const unsigned a = s[3];
				if (a == 0) { d[0] = d[1] = d[2] = d[3] = 0; continue; }
				for (int c = 0; c < 3; c++) d[c] = uint8_t(std::min(255u, (s[c] * 255u + a / 2) / a));
				d[3] = uint8_t(a);
			}
		}
		return savePng(path, PW, PH, image.data());
	};

	// ---- render ----
	const double sr = opt.sampleRate;
	std::vector<float> L(maxBlock), R(maxBlock), wavL, wavR;
	float* chans[2] = {L.data(), R.data()};
	AudioBusBuffers outBus{};
	outBus.numChannels = 2;
	outBus.channelBuffers32 = chans;
	size_t nextEvent = 0;
	int64 songSample = 0;  // rendered so far, from song time 0
	std::map<ParamID, double> autoSent;
	int notesHeld = 0;
	std::vector<int> noteOn(128, 0), noteVel(128, 0);
	float framePeak = 0.0f;
	double frameSq = 0.0;
	int64 frameSamples = 0;

	auto addChange = [](ParameterChanges& changes, ParamID id, int32 offset, double v) {
		int32 idx = 0;
		if (auto* q = changes.addParameterData(id, idx)) { int32 p = 0; q->addPoint(offset, v, p); }
	};
	// --automate ramps, at the start of each block
	auto addAutomation = [&](ParameterChanges& changes, double t0) {
		for (const Automation& a : opt.automation) {
			const double v = a.at(t0);
			auto it = autoSent.find(a.id);
			if (it == autoSent.end() || it->second != v) {
				addChange(changes, a.id, 0, v);
				ctrl->setParamNormalized(a.id, v);  // the host tells the controller too
				autoSent[a.id] = v;
			}
		}
	};
	// one process() call of n samples; keep: the output goes to the WAV and the frame's level
	auto runBlock = [&](int32 n, EventList& evList, ParameterChanges& changes, ProcessContext* ctx, bool keep) {
		ProcessData data{};
		data.processMode = mode;
		data.symbolicSampleSize = kSample32;
		data.numSamples = n;
		data.numOutputs = 1;
		data.outputs = &outBus;
		data.inputEvents = &evList;
		data.inputParameterChanges = &changes;
		data.processContext = ctx;
		std::fill(L.begin(), L.end(), 0.0f);
		std::fill(R.begin(), R.end(), 0.0f);
		proc->process(data);
		if (keep) {
			wavL.insert(wavL.end(), L.begin(), L.begin() + n);
			wavR.insert(wavR.end(), R.begin(), R.begin() + n);
			for (int32 i = 0; i < n; i++) {
				framePeak = std::max(framePeak, std::max(std::fabs(L[i]), std::fabs(R[i])));
				frameSq += 0.5 * (double(L[i]) * L[i] + double(R[i]) * R[i]);
			}
			frameSamples += n;
		}
		songSample += n;
	};

	// A take: the host's blocks as recorded, cut where a frame falls inside one. Events keep their
	// sample positions; a block's parameter changes all go to its first piece (where the host put them).
	size_t recBlock = 0;
	int32 recOff = 0;  // samples of take.blocks[recBlock] already rendered
	auto renderTake = [&](int64 target, bool keep) {
		while (recBlock < take.blocks.size()) {
			const RecBlock& b = take.blocks[recBlock];
			if (b.h.numSamples > 0 && songSample >= target) break;  // empty blocks run as soon as they are reached
			const int32 n = int32(std::min<int64>(b.h.numSamples - recOff, std::max<int64>(0, target - songSample)));
			const bool last = recOff + n >= b.h.numSamples;
			EventList evList;
			ParameterChanges changes;
			if (recOff == 0)
				for (const videorec::ParamPoint& p : b.params) {
					addChange(changes, p.id, std::min<int32>(p.offset, std::max<int32>(0, n - 1)), p.value);
					ctrl->setParamNormalized(p.id, p.value);  // as the host does
				}
			addAutomation(changes, double(songSample) / sr);
			for (const videorec::NoteEvent& e : b.events) {
				if (e.offset < recOff || (e.offset >= recOff + n && !last)) continue;
				Event ev{};
				ev.type = uint16(e.type);
				ev.sampleOffset = std::max<int32>(0, std::min<int32>(e.offset - recOff, n - 1));
				const int pitch = e.pitch & 127;
				if (e.type == Event::kNoteOnEvent) {
					ev.noteOn.channel = e.channel; ev.noteOn.pitch = e.pitch; ev.noteOn.velocity = e.value;
					ev.noteOn.noteId = e.noteId; ev.noteOn.tuning = e.tuning; ev.noteOn.length = e.length;
					if (e.value > 0.0f) {
						if (!noteOn[pitch]) notesHeld++;
						noteOn[pitch] = 1;
						noteVel[pitch] = int(std::lround(e.value * 127.0f));
					}
				} else if (e.type == Event::kNoteOffEvent) {
					ev.noteOff.channel = e.channel; ev.noteOff.pitch = e.pitch; ev.noteOff.velocity = e.value;
					ev.noteOff.noteId = e.noteId; ev.noteOff.tuning = e.tuning;
					if (noteOn[pitch]) notesHeld--;
					noteOn[pitch] = 0;
				} else {
					ev.polyPressure.channel = e.channel; ev.polyPressure.pitch = e.pitch;
					ev.polyPressure.pressure = e.value; ev.polyPressure.noteId = e.noteId;
				}
				evList.addEvent(ev);
			}
			ProcessContext ctx{};
			ctx.sampleRate = sr;
			ctx.state = b.h.ctxState;
			ctx.tempo = b.h.tempo;
			ctx.projectTimeSamples = b.h.projectTimeSamples + recOff;
			ctx.projectTimeMusic = b.h.projectTimeMusic + double(recOff) / sr * b.h.tempo / 60.0;
			ctx.barPositionMusic = b.h.barPositionMusic;
			ctx.timeSigNumerator = b.h.timeSigNum;
			ctx.timeSigDenominator = b.h.timeSigDen;
			runBlock(n, evList, changes, b.h.ctxState ? &ctx : nullptr, keep);
			recOff += n;
			if (recOff >= b.h.numSamples) { recBlock++; recOff = 0; }
		}
	};

	auto renderMidi = [&](int64 target, bool keep) {
		while (songSample < target) {
			const int32 n = int32(std::min<int64>(kMaxBlock, target - songSample));
			const double t0 = double(songSample) / sr, t1 = double(songSample + n) / sr;
			EventList evList;
			ParameterChanges changes;
			while (nextEvent < events.size() && events[nextEvent].sec < t1) {
				const midifile::Event& e = events[nextEvent++];
				const int32 off = std::max<int32>(0, std::min<int32>(n - 1, int32(std::llround((e.sec - t0) * sr))));
				if (e.isNoteOn() || e.isNoteOff()) {
					Event ev{};
					ev.sampleOffset = off;
					if (e.isNoteOn()) {
						ev.type = Event::kNoteOnEvent;
						ev.noteOn.channel = int16(e.channel());
						ev.noteOn.pitch = int16(e.d1);
						ev.noteOn.velocity = float(e.d2) / 127.0f;
						ev.noteOn.noteId = -1;
						if (!noteOn[e.d1]) notesHeld++;
						noteOn[e.d1] = 1;
						noteVel[e.d1] = e.d2;
					} else {
						ev.type = Event::kNoteOffEvent;
						ev.noteOff.channel = int16(e.channel());
						ev.noteOff.pitch = int16(e.d1);
						ev.noteOff.noteId = -1;
						if (noteOn[e.d1]) notesHeld--;
						noteOn[e.d1] = 0;
					}
					evList.addEvent(ev);
				} else {  // controllers, pitch bend, aftertouch: as a host maps them (IMidiMapping)
					CtrlNumber cc = -1;
					double v = 0.0;
					if (e.type() == 0xB0) { cc = CtrlNumber(e.d1); v = e.d2 / 127.0; }
					else if (e.type() == 0xE0) { cc = kPitchBend; v = double((e.d2 << 7) | e.d1) / 16383.0; }
					else if (e.type() == 0xD0) { cc = kAfterTouch; v = e.d1 / 127.0; }
					ParamID tag = 0;
					if (cc >= 0 && ctrl->getMidiControllerAssignment(0, int16(e.channel()), cc, tag) == kResultTrue) {
						addChange(changes, tag, off, v);
						ctrl->setParamNormalized(tag, v);
					}
				}
			}
			addAutomation(changes, t0);
			ProcessContext ctx{};
			ctx.sampleRate = sr;
			ctx.tempo = midi.bpmAtSec(t0);
			ctx.projectTimeSamples = songSample;
			ctx.projectTimeMusic = midi.quartersAtSec(t0);
			ctx.timeSigNumerator = midi.timeSigNum;
			ctx.timeSigDenominator = midi.timeSigDen;
			const double bar = 4.0 * midi.timeSigNum / midi.timeSigDen;
			ctx.barPositionMusic = std::floor(ctx.projectTimeMusic / bar) * bar;
			ctx.state = ProcessContext::kPlaying | ProcessContext::kTempoValid | ProcessContext::kProjectTimeMusicValid |
			            ProcessContext::kBarPositionValid | ProcessContext::kTimeSigValid;
			runBlock(n, evList, changes, &ctx, keep);
		}
	};
	auto renderTo = [&](int64 target, bool keep) {
		if (!rec) { renderMidi(target, keep); return; }
		renderTake(target, keep);
		// a block that starts right at the frame: its start-of-block values are what the host shows now
		if (recBlock < take.blocks.size() && recOff == 0)
			for (const videorec::ParamPoint& p : take.blocks[recBlock].params)
				if (p.offset == 0) ctrl->setParamNormalized(p.id, p.value);
	};

	// pre-roll from song time 0 to --start: notes and controllers before it are in place
	const int64 startSample = int64(std::llround(opt.start * sr));
	renderTo(startSample, false);
	proc->onTimer(nullptr);

	const int frames = int(std::ceil((endSec - opt.start) * opt.fps - 1e-9));
	DataTable table;
	const int32 paramCount = ctrl->getParameterCount();
	std::printf("%d frames at %d fps, %d x %d px, %zu layer(s): %s; preset %s\n", frames, opt.fps, PW, PH, opt.layers.size(),
	            narrow(out).c_str(), presetLabel.c_str());
	const ULONGLONG wall0 = GetTickCount64();
	int failures = 0;
	for (int f = 0; f < frames; f++) {
		const double t = double(f) / opt.fps;  // frame time from --start
		renderTo(startSample + int64(std::llround(t * sr)), true);
		proc->onTimer(nullptr);  // scope, voices, playheads, peak -> controller (what the editor shows)
		// The oscillators reseed rand() from the clock on every note (srand(time(NULL)) in their reset;
		// in a host that is the audio thread, whose rand() is separate). Here audio and editor share one
		// thread, so seed the editor's random letters per frame.
		std::srand(unsigned(1 + f));
		for (const Automation& a : opt.automation)  // the editor shows the value at the frame's own time
			ctrl->setParamNormalized(a.id, a.at(opt.start + t));
		uiClock::setManualSeconds(1000.0 + t);
		nv->makeGLCurrent();
		nv->advanceFrame();
		wchar_t name[32];
		swprintf(name, 32, L"%05d.png", f);
		for (const LayerDef& d : opt.layers)
			if (!drawLayer(d.bits, out + L"\\" + widen(d.name) + L"\\" + name)) failures++;

		// data for this frame
		table.set(f, "time", float(t));
		table.set(f, "song.seconds", float(songSeconds(opt.start + t)));
		table.set(f, "song.quarters", float(songQuarters(opt.start + t)));
		table.set(f, "song.bpm", float(songBpm(opt.start + t)));
		table.set(f, "audio.peak", framePeak);
		table.set(f, "audio.rms", frameSamples ? float(std::sqrt(frameSq / double(frameSamples))) : 0.0f);
		framePeak = 0.0f; frameSq = 0.0; frameSamples = 0;
		table.set(f, "midi.notesHeld", float(notesHeld));
		for (int p = 0; p < 128; p++) {
			if (!pitchUsed[p]) continue;
			const uint8 vs = proc->voiceStates[p].load();
			const std::string k = std::to_string(p);
			table.set(f, "note." + k + ".on", float(noteOn[p]));
			table.set(f, "note." + k + ".velocity", noteVel[p] / 127.0f);
			table.set(f, "voice." + k + ".level", (vs & 0x7F) / 127.0f);
			table.set(f, "voice." + k + ".held", float(vs >> 7));
		}
		for (int k = 0; k < kPlayheadSlots; k++) {
			const std::string s = "playhead." + std::to_string(k) + ".";
			table.set(f, s + "pitch", float(proc->phPitch[k].load()));
			table.set(f, s + "age", proc->phAge[k].load());
			table.set(f, s + "releaseAge", proc->phRel[k].load());
			table.set(f, s + "level", proc->phLevel[k].load());
			for (int j = 0; j < 6; j++) table.set(f, s + kSrcNames[j], proc->phSrc[k][j].load());
		}
		for (int32 i = 0; i < paramCount; i++) {
			ParameterInfo info{};
			if (ctrl->getParameterInfo(i, info) != kResultTrue) continue;
			table.set(f, "param." + std::to_string(info.id) + "." + narrow(std::wstring(reinterpret_cast<const wchar_t*>(info.title))),
			          float(ctrl->getParamNormalized(info.id)));
		}
		if (f % 30 == 0 || f == frames - 1)
			std::printf("  frame %d / %d (%.1f s of %.1f s)  %.1f s elapsed\n", f + 1, frames, t, endSec - opt.start,
			            (GetTickCount64() - wall0) / 1000.0);
	}

	// ---- files that describe the frames ----
	renderTo(startSample + int64(std::llround((endSec - opt.start) * sr)), true);  // the audio runs to the end, past the last frame
	saveWav(out + L"\\audio.wav", wavL, wavR, int(sr));
	char header[1024];
	std::string layerList;
	for (const LayerDef& d : opt.layers) layerList += std::string(layerList.empty() ? "" : ", ") + jsonStr(d.name);
	std::snprintf(header, sizeof(header),
	              "  \"tool\": \"Donna Medina film tool\",\n  \"input\": %s,\n  \"preset\": %s,\n  \"fps\": %d,\n  \"frames\": %d,\n"
	              "  \"start_seconds\": %.6f,\n  \"sample_rate\": %d,\n  \"scale\": %.4g,\n  \"width\": %d,\n  \"height\": %d,\n"
	              "  \"layers\": [%s],\n  \"layer_order\": \"bottom to top as listed; PNG, straight alpha; frame N is at start_seconds + N / fps\",\n"
	              "  \"audio\": \"audio.wav: float32 stereo from start_seconds\"",
	              jsonStr(narrow(input)).c_str(), jsonStr(presetLabel).c_str(), opt.fps, frames, opt.start, int(sr), opt.scale, PW, PH,
	              layerList.c_str());
	table.write(out + L"\\data.json", frames, header);

	// ---- clean up ----
	nv->makeGLCurrent();
	glDeleteRenderbuffers(1, &color);
	glDeleteRenderbuffers(1, &depthStencil);
	glDeleteFramebuffers(1, &fbo);
	uiClock::useRealTime();
	view->removed();
	view->release();
	DestroyWindow(hwnd);
	proc->setActive(false);
	static_cast<Vst::IConnectionPoint*>(proc)->disconnect(static_cast<Vst::IConnectionPoint*>(ctrl));
	static_cast<Vst::IConnectionPoint*>(ctrl)->disconnect(static_cast<Vst::IConnectionPoint*>(proc));
	proc->terminate();
	ctrl->terminate();
	proc->release();
	ctrl->release();
	g_wic->Release();
	std::printf("done: %d frames, %.1f s; %s\n", frames, (GetTickCount64() - wall0) / 1000.0, failures ? "SOME LAYERS FAILED" : "ok");
	return failures ? 2 : 0;
}
