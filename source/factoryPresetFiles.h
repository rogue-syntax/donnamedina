#pragma once
// Factory presets kept as .vstpreset files (2026-11-20): presets/factory/*.vstpreset, embedded in the
// plugin as RCDATA resources (RSSynth001donnaMedina.rc, "FACTORY PRESET FILES"). They follow the
// built-in factory programs (DX10Processor::fillFactoryPresets reads them into programParams), so the
// host's program list, the preset bar and saved projects treat them like the others.
//
// To add one: copy the file into presets/factory, add its RCDATA line to the .rc and its entry here,
// and raise kNumPrograms (mdaBaseProcessor.h). Keep the order: a project stores the program number.

namespace factoryPresetFiles {

struct Entry {
	const char* name;      // shown in the preset bar and the host's program list
	const char* resource;  // RCDATA name in the .rc
};

static const Entry kFiles[] = {
	{"303-1", "PRESET_303_1"},
	{"303-2", "PRESET_303_2"},
	{"303-3", "PRESET_303_3"},
	{"Polycrystalline-2", "PRESET_POLYCRYSTALLINE_2"},
	{"taiko-1", "PRESET_TAIKO_1"},
	{"Teardrop-1", "PRESET_TEARDROP_1"},
};
constexpr int kCount = int(sizeof(kFiles) / sizeof(kFiles[0]));
constexpr int kBuiltIn = 7;  // the factory programs written in code come first

} // namespace factoryPresetFiles
