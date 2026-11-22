#pragma once
// Record-for-video (2026-11-22). Never in a release; see docs/08-video.md.
//
// A recording build of the plugin logs everything the host feeds the processor while the host
// renders offline (an export): the processor's state at the start, then every block's transport
// (tempo, song position), parameter changes and note events, with their sample offsets. The film
// tool replays the log (film.exe --record take.rsrec) to film the editor as it was during the export.
// Takes go to Documents\Donna Medina video\ (or %DONNA_VIDEO_DIR%), one file per export.
// A recording build shows a small red light after "GENERATOR CONTROL" (bright while recording).
//
// To make a recording build, uncomment the next line and build. Comment it out again afterwards:
// installer\release.ps1 refuses to package a recording build.
// #define RS_VIDEO_RECORD

#include <cstdint>

namespace videorec {

// ---- the take file (.rsrec), little-endian, packed -------------------------------------------
//   FileHeader, writer[32] (text), state[stateSize] (BaseProcessor::getState), then per block:
//   BlockHeader, ParamPoint[numParams], NoteEvent[numEvents], to the end of the file.
constexpr char kMagic[8] = {'R', 'S', 'V', 'R', 'E', 'C', '1', '\n'};
constexpr uint32_t kVersion = 1;

#pragma pack(push, 1)
struct FileHeader {
	char magic[8];
	uint32_t version;
	double sampleRate;
	int32_t maxBlock;   // ProcessSetup::maxSamplesPerBlock
	uint32_t stateSize;
};
struct BlockHeader {
	int64_t projectTimeSamples;
	int32_t numSamples;
	uint32_t ctxState;  // ProcessContext::state, 0 without a context
	double tempo, projectTimeMusic, barPositionMusic;
	int32_t timeSigNum, timeSigDen;
	int32_t numParams, numEvents;
};
struct ParamPoint {
	uint32_t id;
	int32_t offset;
	double value;
};
struct NoteEvent {
	int32_t type;       // Event::kNoteOnEvent, kNoteOffEvent, kPolyPressureEvent
	int32_t offset;     // sampleOffset in the block
	int16_t channel, pitch;
	float value;        // velocity (on/off) or pressure
	int32_t noteId;
	float tuning;
	int32_t length;
};
#pragma pack(pop)

} // namespace videorec

#ifdef RS_VIDEO_RECORD
#include <vector>
namespace Steinberg { namespace Vst { struct ProcessData; struct ProcessSetup; class AudioEffect; } }

namespace videorec {

// One per processor. Everything runs on the thread that calls it (the host's audio thread for
// block(); offline rendering has no real-time deadline, so a take is collected in memory).
class Recorder {
public:
	~Recorder() { finish(false); }
	// at the start of every process() call
	void block(const Steinberg::Vst::ProcessData& data, const Steinberg::Vst::ProcessSetup& setup,
	           Steinberg::Vst::AudioEffect& component);
	// end the take and write it; async: on a thread of its own (from the audio thread)
	void finish(bool async);

private:
	std::vector<uint8_t> buf;
	bool open = false;
};

// a take is being recorded in some instance (the editor's light)
bool recordingNow();

} // namespace videorec
#endif
