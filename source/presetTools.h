#pragma once
// Preset bar helpers (2026-11-14): the plugin state blob, the randomizer and the user preset folder.
// No VST3 or GUI objects here, so tests can use them directly.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace presetTools {

// The processor state as BaseProcessor::getState writes it (little endian):
//   uint32 magic (BaseController::kMagicNumber), uint32 program, uint32 count, count x double,
//   uint32 bypass, then optionally uint32 MASTER_VOL_STATE_TAG + double master volume.
// States without the magic start directly with the count (very old format; parse accepts it).
struct State {
	bool hasHeader = true;
	uint32_t program = 0;
	std::vector<double> values;
	uint32_t bypass = 0;
	bool hasMasterVol = false;
	double masterVol = 0.0;
};

bool parseState(const void* data, size_t size, State& out);
std::vector<char> writeState(const State& s);

// Randomizer. stepCounts[pid] for pid < count: the parameter's step count (0 = continuous) or -1 when
// the id is not a randomizable parameter. Ids the rules keep (pitch of the carrier, master volume,
// buttons, ...) are left alone either way.
//   kVariation: nudges the continuous parameters the rules allow by a small amount; switches stay.
//   kFull:      a new patch: every allowed parameter random within its safe range.
enum class Mode { kVariation, kFull };
void randomize(State& s, const int* stepCounts, size_t count, Mode mode, uint32_t seed);

// Documents\VST3 Presets\Rogue Syntax\Donna Medina (the standard VST3 user preset location).
std::wstring userPresetDir();

// Every *.vstpreset under dir (subfolders included), sorted by file name (case-insensitive).
std::vector<std::wstring> listPresetFiles(const std::wstring& dir);

// File name without folder and extension, as UTF-8.
std::string presetDisplayName(const std::wstring& path);

// Path helpers (the project builds as C++14: no std::filesystem).
std::wstring parentDir(const std::wstring& path);         // "" when there is none
bool hasExtension(const std::wstring& path, const wchar_t* ext);  // ext like L".vstpreset"
bool ensureDir(const std::wstring& dir);                 // creates missing folders

bool readFile(const std::wstring& path, std::vector<char>& out);
bool writeFile(const std::wstring& path, const std::vector<char>& data);  // creates folders

std::string toUtf8(const std::wstring& w);
std::wstring fromUtf8(const std::string& s);

} // namespace presetTools
