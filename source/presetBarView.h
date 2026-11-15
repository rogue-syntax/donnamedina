#pragma once
// Preset bar (2026-11-14): an invisible view over the bar in the right column (custom view
// "presetBar" in vstgui.uidesc, created by BaseController::createCustomView). It only handles the
// mouse: the zone under the cursor goes to BaseController::presetHover (NVGView draws the hover),
// clicks go to BaseController::onPresetBarClick. NVGView draws the bar itself (drawPresetBar).

#include "vstgui/vstgui.h"
#include "editorLayout.h"

namespace presetBar {

enum Zone { kNone = -1, kPrev = 0, kName, kNext, kSave, kRnd, kZoneCount };

// Zone rectangles in window coordinates (inside the PRESET inset: x 937..1231, y 80..102).
constexpr editorLayout::column::Rect kBar {937.0f, 80.0f, 294.0f, 22.0f};
constexpr editorLayout::column::Rect kZones[kZoneCount] = {
	{937.0f, 80.0f, 22.0f, 22.0f},    // <
	{961.0f, 80.0f, 172.0f, 22.0f},   // name (opens the preset menu)
	{1135.0f, 80.0f, 22.0f, 22.0f},   // >
	{1161.0f, 80.0f, 36.0f, 22.0f},   // SAVE
	{1199.0f, 80.0f, 32.0f, 22.0f},   // RND
};

inline int zoneAt(float x, float y) {
	for (int z = 0; z < kZoneCount; z++) {
		const auto& r = kZones[z];
		if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
			return z;
	}
	return kNone;
}

} // namespace presetBar

namespace Steinberg { namespace Vst { namespace rs {

class BaseController;

class PresetBarView : public VSTGUI::CView {
public:
	PresetBarView(const VSTGUI::CRect& size, BaseController* c);
	~PresetBarView() override;

	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	void draw(VSTGUI::CDrawContext*) override {}  // drawn by NVGView

private:
	int zoneFor(const VSTGUI::CPoint& where) const;
	BaseController* ctrl;
};

}}} // namespaces
