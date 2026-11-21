// Preset bar (2026-11-14): DX10Controller's preset list, loading, saving and randomizing.
//
// Every change goes through the plugin state, the way a host loads a preset: the controller builds
// the processor state blob, sends it to the processor (CMSG_SET_STATE_CHAR, applied with setState on
// the main thread) and applies the same blob to itself with setComponentState. Saving and
// randomizing first ask the processor for its current state (CMSG_GET_STATE_CHAR -> PMSG_STATE_CHAR).
// Master volume and bypass are global: every load keeps the current ones.

#include "mdaDX10Controller.h"
#include "presetTools.h"
#include "presetBarView.h"
#include "factoryPresetFiles.h"

#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "base/source/fstreamer.h"

#include <chrono>

#include <windows.h>
#include <shellapi.h>

namespace Steinberg {
namespace Vst {
namespace rs {

namespace {
const uint32 kCtrlStateTag = 0x52414250;  // "PBAR": controller state block (preset bar)
const uint32 kCtrlStateVersion = 1;
}

const char* DX10Controller::factoryPresetName(int i) {
	// the programs written in code, then the preset files (factoryPresetFiles.h)
	static const char* names[] = {"Softly", "Prima Donna", "Perturb", "Polycrystalline", "Skillsaw",
	                              "Brooding Sky Arp", "Brooding Sea Strings"};
	static_assert(sizeof(names) / sizeof(names[0]) == factoryPresetFiles::kBuiltIn, "factory names");
	if (i >= 0 && i < factoryPresetFiles::kBuiltIn)
		return names[i];
	if (i >= factoryPresetFiles::kBuiltIn && i < DX10Processor::kNumPrograms)
		return factoryPresetFiles::kFiles[i - factoryPresetFiles::kBuiltIn].name;
	return "";
}

void DX10Controller::rescanPresets() {
	presetList.clear();
	for (int i = 0; i < DX10Processor::kNumPrograms; i++) {
		PresetEntry e;
		e.name = factoryPresetName(i);
		e.factory = i;
		presetList.push_back(e);
	}
	for (const auto& p : presetTools::listPresetFiles(presetTools::userPresetDir())) {
		PresetEntry e;
		e.name = presetTools::presetDisplayName(p);
		e.path = p;
		presetList.push_back(e);
	}
	syncPresetCursor();
}

void DX10Controller::syncPresetCursor() {
	presetCursor = -1;
	for (int i = 0; i < int(presetList.size()); i++) {
		const PresetEntry& e = presetList[i];
		if ((presetFactory >= 0 && e.factory == presetFactory)
			|| (!presetPath.empty() && e.factory < 0 && _wcsicmp(e.path.c_str(), presetPath.c_str()) == 0)) {
			presetCursor = i;
			return;
		}
	}
}

void DX10Controller::presetStepCounts(std::vector<int>& out) {
	out.assign(GPARAMS, -1);
	for (int pid = 0; pid < GPARAMS && pid <= FX_PARAM_LAST; pid++) {
		if (!isHostParam(pid))
			continue;
		if (isGlobalParam(pid))
			continue;
		Parameter* p = EditController::getParameterObject(pid);
		if (!p)
			continue;
		out[pid] = p->getInfo().stepCount;
	}
}

void DX10Controller::applyPresetState(presetTools::State s) {
	// globals stay as they are
	Parameter* bypass = parameters.getParameter(kBypassParam);
	s.bypass = (bypass && bypass->getNormalized() > 0.5) ? 1 : 0;
	const double vol = EditController::getParamNormalized(MASTER_VOL);
	s.hasMasterVol = true;
	s.masterVol = vol;
	if (s.values.size() > size_t(MASTER_VOL))
		s.values[MASTER_VOL] = vol;
	const double poly = EditController::getParamNormalized(POLY_COUNT);
	s.hasPoly = true;
	s.poly = poly;
	if (s.values.size() > size_t(POLY_COUNT))
		s.values[POLY_COUNT] = poly;
	const double hq = EditController::getParamNormalized(HQ_MODE);
	s.hasHq = true;
	s.hq = hq;
	if (s.values.size() > size_t(HQ_MODE))
		s.values[HQ_MODE] = hq;
	std::vector<char> blob = presetTools::writeState(s);

	if (IMessage* msg = allocateMessage()) {
		msg->setMessageID(msgIdStr(CMSG_SET_STATE_CHAR));
		msg->getAttributes()->setBinary("data", blob.data(), uint32(blob.size()));
		sendMessage(msg);
		msg->release();
	}
	MemoryStream ms(blob.data(), TSize(blob.size()));
	applyingPresetState = true;  // the caller names the preset
	setComponentState(&ms);
	applyingPresetState = false;
	if (componentHandler)
		componentHandler->restartComponent(kParamValuesChanged);
}

void DX10Controller::loadFactoryPreset(int index) {
	if (index < 0 || index >= DX10Processor::kNumPrograms)
		return;
	DX10Processor::ensureFactoryPresets();
	presetTools::State s;
	s.hasHeader = true;
	s.program = uint32(index);
	s.values.resize(GPARAMS);
	for (int i = 0; i < GPARAMS; i++)
		s.values[i] = DX10Processor::programParams[index][i];
	applyPresetState(s);
	presetFactory = index;
	presetPath.clear();
	presetName = factoryPresetName(index);
	presetDirty = false;
	syncPresetCursor();
}

bool DX10Controller::loadPresetFile(const std::wstring& path) {
	std::vector<char> file;
	if (!presetTools::readFile(path, file) || file.empty())
		return false;
	MemoryStream fs(file.data(), TSize(file.size()));
	PresetFile pf(&fs);
	if (!pf.readChunkList() || pf.getClassID() != DX10Processor::uid)
		return false;
	const PresetFile::Entry* e = pf.getEntry(kComponentState);
	if (!e || e->size <= 0 || e->offset < 0 || e->offset + e->size > TSize(file.size()))
		return false;
	presetTools::State s;
	if (!presetTools::parseState(file.data() + e->offset, size_t(e->size), s))
		return false;
	applyPresetState(s);
	presetFactory = -1;
	presetPath = path;
	presetName = presetTools::presetDisplayName(path);
	presetDirty = false;
	rescanPresets();  // the file may be outside the list (Open file...) or new
	return true;
}

void DX10Controller::loadPresetEntry(int listIndex) {
	if (listIndex < 0 || listIndex >= int(presetList.size()))
		return;
	const PresetEntry e = presetList[listIndex];
	if (e.factory >= 0)
		loadFactoryPreset(e.factory);
	else if (!loadPresetFile(e.path))
		rescanPresets();  // the file is gone or unreadable: drop it from the list
}

void DX10Controller::stepPreset(int dir) {
	rescanPresets();
	const int n = int(presetList.size());
	if (n == 0)
		return;
	int next = presetCursor < 0 ? (dir > 0 ? 0 : n - 1) : (presetCursor + dir + n) % n;
	loadPresetEntry(next);
}

void DX10Controller::requestProcessorState(int purpose) {
	if (IMessage* msg = allocateMessage()) {
		msg->setMessageID(msgIdStr(CMSG_GET_STATE_CHAR));
		msg->getAttributes()->setInt("purpose", purpose);
		sendMessage(msg);
		msg->release();
	}
}

void DX10Controller::onProcessorState(const void* data, uint32 size, int64 purpose) {
	presetTools::State s;
	if (!presetTools::parseState(data, size, s))
		return;
	if (purpose == kStateForSave) {
		const std::wstring path = pendingSavePath;
		pendingSavePath.clear();
		if (path.empty())
			return;
		// write exactly what the processor has (master volume included; this plugin ignores it on load)
		MemoryStream comp(const_cast<void*>(data), TSize(size));
		MemoryStream out;
		if (!PresetFile::savePreset(&out, DX10Processor::uid, &comp))
			return;
		std::vector<char> bytes(out.getData(), out.getData() + out.getSize());
		if (!presetTools::writeFile(path, bytes))
			return;
		presetFactory = -1;
		presetPath = path;
		presetName = presetTools::presetDisplayName(path);
		presetDirty = false;
		rescanPresets();
		return;
	}
	if (purpose == kStateForVariation || purpose == kStateForRandom) {
		std::vector<int> steps;
		presetStepCounts(steps);
		const uint32_t seed = uint32_t(std::chrono::steady_clock::now().time_since_epoch().count()) ^ (++randomCount * 2654435761u);
		const bool full = purpose == kStateForRandom;
		presetTools::randomize(s, steps.data(), steps.size(),
		                       full ? presetTools::Mode::kFull : presetTools::Mode::kVariation, seed);
		applyPresetState(s);
		if (full) {
			presetFactory = -1;
			presetPath.clear();
			presetName = "Random";
			presetDirty = false;
			syncPresetCursor();
		}
		else {
			presetDirty = true;
		}
	}
}

void DX10Controller::runSaveDialog(CFrame* frame) {
	CNewFileSelector* sel = CNewFileSelector::create(frame, CNewFileSelector::kSelectSaveFile);
	if (!sel)
		return;
	const std::wstring dir = presetTools::userPresetDir();
	presetTools::ensureDir(dir);
	const std::wstring startDir = presetPath.empty() ? dir : presetTools::parentDir(presetPath);
	sel->setTitle("Save Preset");
	sel->setInitialDirectory(presetTools::toUtf8(startDir).c_str());
	sel->setDefaultSaveName((presetName.empty() ? std::string("My Preset") : presetName).c_str());
	sel->setDefaultExtension(CFileExtension("VST3 Preset", "vstpreset"));
	sel->run([this](CNewFileSelector* s) {
		if (s->getNumSelectedFiles() == 0)
			return;
		std::wstring path = presetTools::fromUtf8(s->getSelectedFile(0));
		if (!presetTools::hasExtension(path, L".vstpreset"))
			path += L".vstpreset";
		pendingSavePath = path;
		requestProcessorState(kStateForSave);
	});
	sel->forget();
}

void DX10Controller::runOpenDialog(CFrame* frame) {
	CNewFileSelector* sel = CNewFileSelector::create(frame, CNewFileSelector::kSelectFile);
	if (!sel)
		return;
	const std::wstring dir = presetTools::userPresetDir();
	sel->setTitle("Open Preset");
	sel->setInitialDirectory(presetTools::toUtf8(dir).c_str());
	sel->setDefaultExtension(CFileExtension("VST3 Preset", "vstpreset"));
	sel->run([this](CNewFileSelector* s) {
		if (s->getNumSelectedFiles() > 0)
			loadPresetFile(presetTools::fromUtf8(s->getSelectedFile(0)));
	});
	sel->forget();
}

void DX10Controller::showPresetMenu(CFrame* frame, const CPoint& where) {
	rescanPresets();
	enum { kActLoad, kActRandom, kActSave, kActOpen, kActFolder, kActNone };
	struct Item { int act; int index; };
	std::vector<Item> items;
	auto menu = makeOwned<COptionMenu>();
	auto add = [&](const std::string& title, int flags, Item it) {
		menu->addEntry(title.c_str(), -1, flags);
		items.push_back(it);
	};
	add("FACTORY", CMenuItem::kTitle | CMenuItem::kDisabled, {kActNone, -1});
	bool userHeader = false;
	for (int i = 0; i < int(presetList.size()); i++) {
		if (presetList[i].factory < 0 && !userHeader) {
			menu->addSeparator();
			items.push_back({kActNone, -1});
			add("USER", CMenuItem::kTitle | CMenuItem::kDisabled, {kActNone, -1});
			userHeader = true;
		}
		add("   " + presetList[i].name, i == presetCursor ? CMenuItem::kChecked : CMenuItem::kNoFlags, {kActLoad, i});
	}
	menu->addSeparator();
	items.push_back({kActNone, -1});
	add("Randomize all", CMenuItem::kNoFlags, {kActRandom, -1});
	add("Save as...", CMenuItem::kNoFlags, {kActSave, -1});
	add("Open file...", CMenuItem::kNoFlags, {kActOpen, -1});
	add("Show preset folder", CMenuItem::kNoFlags, {kActFolder, -1});

	menu->popup(frame, where, [this, items, frame](COptionMenu* m) {
		const int r = m->getLastResult();
		if (r < 0 || r >= int(items.size()))
			return;
		const Item it = items[r];
		switch (it.act) {
			case kActLoad: loadPresetEntry(it.index); break;
			case kActRandom: requestProcessorState(kStateForRandom); break;
			case kActSave: runSaveDialog(frame); break;
			case kActOpen: runOpenDialog(frame); break;
			case kActFolder: {
				const std::wstring dir = presetTools::userPresetDir();
				presetTools::ensureDir(dir);
				ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
				break;
			}
			default: break;
		}
	});
}

void DX10Controller::onPresetBarClick(int zone, CFrame* frame, const CPoint& where) {
	switch (zone) {
		case presetBar::kPrev: stepPreset(-1); break;
		case presetBar::kNext: stepPreset(+1); break;
		case presetBar::kName: {
			// open the menu under the name's left edge
			const auto& r = presetBar::kZones[presetBar::kName];
			CPoint p(r.x, r.y + r.h);
			(void)where;
			if (frame) showPresetMenu(frame, p);
			break;
		}
		case presetBar::kSave: if (frame) runSaveDialog(frame); break;
		case presetBar::kRnd: requestProcessorState(kStateForVariation); break;
		default: break;
	}
}

// ---- controller state: which preset is loaded (the processor state holds the sound) ----

tresult PLUGIN_API DX10Controller::setState(IBStream* state) {
	if (!state)
		return kResultFalse;
	IBStreamer s(state, kLittleEndian);
	uint32 tag = 0, version = 0, nameLen = 0, pathLen = 0;
	int32 factory = -1;
	int8 dirty = 0;
	if (!s.readInt32u(tag) || tag != kCtrlStateTag || !s.readInt32u(version) || version < 1)
		return kResultOk;  // no preset bar block: keep what setComponentState set
	if (!s.readInt32(factory) || !s.readInt8(dirty) || !s.readInt32u(nameLen) || nameLen > 4096)
		return kResultOk;
	std::string name(nameLen, '\0');
	if (nameLen && s.readRaw(&name[0], int32(nameLen)) != int32(nameLen))
		return kResultOk;
	if (!s.readInt32u(pathLen) || pathLen > 32768)
		return kResultOk;
	std::string path(pathLen, '\0');
	if (pathLen && s.readRaw(&path[0], int32(pathLen)) != int32(pathLen))
		return kResultOk;
	presetFactory = (factory >= 0 && factory < DX10Processor::kNumPrograms) ? factory : -1;
	presetPath = presetTools::fromUtf8(path);
	presetName = name;
	presetDirty = dirty != 0;
	rescanPresets();
	return kResultOk;
}

tresult PLUGIN_API DX10Controller::getState(IBStream* state) {
	if (!state)
		return kResultFalse;
	IBStreamer s(state, kLittleEndian);
	const std::string path = presetTools::toUtf8(presetPath);
	s.writeInt32u(kCtrlStateTag);
	s.writeInt32u(kCtrlStateVersion);
	s.writeInt32(presetFactory);
	s.writeInt8(int8(presetDirty ? 1 : 0));
	s.writeInt32u(uint32(presetName.size()));
	if (!presetName.empty()) s.writeRaw(presetName.data(), int32(presetName.size()));
	s.writeInt32u(uint32(path.size()));
	if (!path.empty()) s.writeRaw(path.data(), int32(path.size()));
	return kResultOk;
}

tresult PLUGIN_API DX10Controller::setComponentState(IBStream* state) {
	const tresult r = BaseController::setComponentState(state);
	// A host loading a project or its own preset: show the factory program the state names. The
	// controller state (setState, called after this) replaces it with the saved preset bar name.
	if (Parameter* p = !applyingPresetState ? parameters.getParameter(kPresetParam) : nullptr) {
		const int prog = int(p->toPlain(p->getNormalized()) + 0.5);
		presetFactory = prog;
		presetPath.clear();
		presetName = factoryPresetName(prog);
		presetDirty = false;
		syncPresetCursor();
	}
	return r;
}

}}} // namespaces
