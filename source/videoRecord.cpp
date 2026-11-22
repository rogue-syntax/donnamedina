// Record-for-video: the take writer. Compiled to nothing unless RS_VIDEO_RECORD is defined in
// videoRecord.h (never in a release). See videoRecord.h and docs/08-video.md.
#include "videoRecord.h"

#ifdef RS_VIDEO_RECORD
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/common/memorystream.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <windows.h>
#include <shlobj.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace videorec {

namespace {
std::atomic<int> g_open{0};
std::atomic<int> g_seq{0};

// what release.ps1 looks for in the built plugin; written into every take
const char kWriter[32] = "DONNA-MEDINA-VIDEO-RECORD-BUILD";

template <typename T> void put(std::vector<uint8_t>& b, const T& v) {
	const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
	b.insert(b.end(), p, p + sizeof(T));
}

std::wstring takeFolder() {
	wchar_t env[MAX_PATH];
	const DWORD n = GetEnvironmentVariableW(L"DONNA_VIDEO_DIR", env, MAX_PATH);
	if (n > 0 && n < MAX_PATH) return env;
	PWSTR docs = nullptr;
	std::wstring dir;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs))) dir = docs;
	CoTaskMemFree(docs);
	return dir.empty() ? std::wstring() : dir + L"\\Donna Medina video";
}

void writeTake(std::vector<uint8_t> data) {
	const std::wstring dir = takeFolder();
	if (dir.empty()) return;
	CreateDirectoryW(dir.c_str(), nullptr);
	std::time_t t = std::time(nullptr);
	std::tm lt{};
	localtime_s(&lt, &t);
	wchar_t name[96];
	swprintf(name, 96, L"\\take-%04d%02d%02d-%02d%02d%02d-%d.rsrec", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
	         lt.tm_hour, lt.tm_min, lt.tm_sec, ++g_seq);
	FILE* f = _wfopen((dir + name).c_str(), L"wb");
	if (!f) return;
	std::fwrite(data.data(), 1, data.size(), f);
	std::fclose(f);
}
} // namespace

bool recordingNow() { return g_open.load(std::memory_order_relaxed) > 0; }

void Recorder::block(const ProcessData& data, const ProcessSetup& setup, AudioEffect& component) {
	if (data.processMode != kOffline) {
		if (open) finish(true);  // the export is over
		return;
	}
	if (!open) {
		buf.clear();
		buf.reserve(size_t(8) << 20);
		MemoryStream ms;
		component.getState(&ms);
		FileHeader h{};
		std::memcpy(h.magic, kMagic, sizeof(kMagic));
		h.version = kVersion;
		h.sampleRate = setup.sampleRate;
		h.maxBlock = setup.maxSamplesPerBlock;
		h.stateSize = uint32_t(ms.getSize());
		put(buf, h);
		buf.insert(buf.end(), reinterpret_cast<const uint8_t*>(kWriter), reinterpret_cast<const uint8_t*>(kWriter) + sizeof(kWriter));
		const uint8_t* s = reinterpret_cast<const uint8_t*>(ms.getData());
		if (h.stateSize) buf.insert(buf.end(), s, s + h.stateSize);
		open = true;
		g_open++;
	}

	BlockHeader bh{};
	bh.numSamples = data.numSamples;
	if (const ProcessContext* c = data.processContext) {
		bh.projectTimeSamples = c->projectTimeSamples;
		bh.ctxState = c->state;
		bh.tempo = c->tempo;
		bh.projectTimeMusic = c->projectTimeMusic;
		bh.barPositionMusic = c->barPositionMusic;
		bh.timeSigNum = c->timeSigNumerator;
		bh.timeSigDen = c->timeSigDenominator;
	}
	const size_t at = buf.size();
	put(buf, bh);
	int32_t np = 0, ne = 0;
	if (IParameterChanges* pc = data.inputParameterChanges) {
		for (int32 i = 0; i < pc->getParameterCount(); i++) {
			IParamValueQueue* q = pc->getParameterData(i);
			if (!q) continue;
			for (int32 j = 0; j < q->getPointCount(); j++) {
				int32 off = 0;
				ParamValue v = 0;
				if (q->getPoint(j, off, v) != kResultTrue) continue;
				put(buf, ParamPoint{q->getParameterId(), off, v});
				np++;
			}
		}
	}
	if (IEventList* el = data.inputEvents) {
		for (int32 i = 0; i < el->getEventCount(); i++) {
			Event e{};
			if (el->getEvent(i, e) != kResultTrue) continue;
			NoteEvent n{};
			n.type = e.type;
			n.offset = e.sampleOffset;
			if (e.type == Event::kNoteOnEvent) {
				n.channel = e.noteOn.channel; n.pitch = e.noteOn.pitch; n.value = e.noteOn.velocity;
				n.noteId = e.noteOn.noteId; n.tuning = e.noteOn.tuning; n.length = e.noteOn.length;
			} else if (e.type == Event::kNoteOffEvent) {
				n.channel = e.noteOff.channel; n.pitch = e.noteOff.pitch; n.value = e.noteOff.velocity;
				n.noteId = e.noteOff.noteId; n.tuning = e.noteOff.tuning;
			} else if (e.type == Event::kPolyPressureEvent) {
				n.channel = e.polyPressure.channel; n.pitch = e.polyPressure.pitch; n.value = e.polyPressure.pressure;
				n.noteId = e.polyPressure.noteId;
			} else continue;  // the synth plays only notes; other event types carry pointers
			put(buf, n);
			ne++;
		}
	}
	BlockHeader* stored = reinterpret_cast<BlockHeader*>(buf.data() + at);
	stored->numParams = np;
	stored->numEvents = ne;
}

void Recorder::finish(bool async) {
	if (!open) return;
	open = false;
	g_open--;
	if (async) std::thread(writeTake, std::move(buf)).detach();
	else writeTake(std::move(buf));
	buf = std::vector<uint8_t>();
}

} // namespace videorec
#endif
