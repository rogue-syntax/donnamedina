#pragma once
// Preset bar (2026-11-14): an invisible view over the bar in the right column (custom view
// "presetBar" in vstgui.uidesc, created by BaseController::createCustomView). It only handles the
// mouse: the zone under the cursor goes to BaseController::presetHover (NVGView draws the hover),
// clicks go to BaseController::onPresetBarClick. NVGView draws the bar itself (drawPresetBar).

#include "vstgui/vstgui.h"
#include "editorLayout.h"

namespace presetBar {

enum Zone { kNone = -1, kPrev = 0, kName, kNext, kSave, kRnd, kZoneCount };

// Zone rectangles in window coordinates (the PRESET inset: x column::kBoxL..kBoxR, y 80..102 + column::kDY).
constexpr float kX = editorLayout::column::kBoxL;
constexpr float kY = 80.0f + editorLayout::column::kDY;
constexpr editorLayout::column::Rect kBar {kX, kY, editorLayout::column::kBoxR - kX, 22.0f};  // 920,96 315x22
constexpr editorLayout::column::Rect kZones[kZoneCount] = {
	{kX, kY, 22.0f, 22.0f},            // <
	{kX + 24.0f, kY, 193.0f, 22.0f},   // name (opens the preset menu)
	{kX + 219.0f, kY, 22.0f, 22.0f},   // >
	{kX + 245.0f, kY, 36.0f, 22.0f},   // SAVE
	{kX + 283.0f, kY, 32.0f, 22.0f},   // RND (ends at kBoxR)
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

// Polyphony (2026-11-15): the scope's "VOICES n/LIMIT" plate (top left of the scope's inner area) is
// an invisible view too (custom view "polyCount"): drag up/down or the mouse wheel changes the limit,
// a click without dragging opens a menu of common values. NVGView draws the plate (drawScopeInfo).
namespace polyZone {
constexpr editorLayout::column::Rect kRect {editorLayout::column::kBoxL, 201.0f + editorLayout::column::kDY, 132.0f, 13.0f};
}

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

// About screen links (2026-11-18): an invisible view over the links row (custom view "aboutLinks",
// editorLayout::about::kLinks). NVGView enables its mouse only while the about screen is open (it
// sits over panel controls). The hovered zone goes to BaseController::aboutHover (NVGView draws
// it); a click opens the website or a mail to support.
namespace aboutLinks {
// kPasteLic .. kUpdHide (2026-11-20): Paste license, the Check for updates box and the update line
enum Zone { kNone = -1, kWeb = 0, kMail, kTabAbout, kTabCredits, kPasteLic, kUpdToggle, kUpdLink, kUpdHide, kZoneCount };
// x, y in window coordinates; the links and controls only exist on the about page (not on the
// credits page); the update line's zones only while it is shown (updateLine)
inline int zoneAt(float x, float y, bool creditsPage, bool updateLine = false, bool hideOption = false) {
	using namespace editorLayout::about;
	const editorLayout::ImageRect z[kZoneCount] = {kWebZone, kMailZone, editorLayout::about::kTabAbout,
	                                               editorLayout::about::kTabCredits, editorLayout::about::kPasteLic,
	                                               editorLayout::about::kUpdToggle, editorLayout::about::kUpdLink,
	                                               editorLayout::about::kUpdHide};
	for (int i = 0; i < kZoneCount; i++) {
		if (creditsPage && i != kTabAbout && i != kTabCredits)
			continue;
		if ((i == kUpdLink && !updateLine) || (i == kUpdHide && !(updateLine && hideOption)))
			continue;
		if (x >= z[i].x && x < z[i].x + z[i].w && y >= z[i].y && y < z[i].y + z[i].h)
			return i;
	}
	return kNone;
}
constexpr const wchar_t* kUrl[2] = {L"http://www.rogue-syntax.com", L"mailto:admin@rogue-syntax.com"};
} // namespace aboutLinks

class AboutLinksView : public VSTGUI::CView {
public:
	AboutLinksView(const VSTGUI::CRect& size, BaseController* c);
	~AboutLinksView() override;

	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	bool onWheel(const VSTGUI::CPoint& where, const VSTGUI::CMouseWheelAxis& axis, const float& distance,
	             const VSTGUI::CButtonState& buttons) override;
	void draw(VSTGUI::CDrawContext*) override {}  // drawn by NVGView

private:
	int zoneFor(const VSTGUI::CPoint& where) const;
	BaseController* ctrl;
};

// "Not registered" screen (licensing v1): an invisible view over editorLayout::licenseScreen::kBox, on
// top of everything and mouse-enabled only while unlicensed. Buy opens the shop page; Paste license
// pastes a license from the clipboard (BaseController::pasteLicense). NVGView draws it all.
namespace licenseZone {
enum Zone { kNone = -1, kBuy = 0, kPaste, kZoneCount };
inline int zoneAt(float x, float y) {
	// Spelled out: inside this namespace kBuy / kPaste are the Zone values (0, 1). With a using-directive
	// those won the lookup, the rectangles became {0, 1, 0, 0}, and Buy / Paste license never took a
	// click (fixed 2026-11-21).
	const editorLayout::ImageRect z[kZoneCount] = {editorLayout::licenseScreen::kBuy, editorLayout::licenseScreen::kPaste};
	for (int i = 0; i < kZoneCount; i++)
		if (x >= z[i].x && x < z[i].x + z[i].w && y >= z[i].y && y < z[i].y + z[i].h)
			return i;
	return kNone;
}
} // namespace licenseZone

class LicenseView : public VSTGUI::CView {
public:
	LicenseView(const VSTGUI::CRect& size, BaseController* c);
	~LicenseView() override;

	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	void draw(VSTGUI::CDrawContext*) override {}  // drawn by NVGView

private:
	int zoneFor(const VSTGUI::CPoint& where) const;
	BaseController* ctrl;
};

// "Update available" badge (update check): an invisible view over editorLayout::kUpdateBadge,
// mouse-enabled only while the badge is shown (NVGView); a click opens the URL of the last answer.
class UpdateBadgeView : public VSTGUI::CView {
public:
	UpdateBadgeView(const VSTGUI::CRect& size, BaseController* c);
	~UpdateBadgeView() override;

	VSTGUI::CMouseEventResult onMouseEntered(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	void draw(VSTGUI::CDrawContext*) override {}  // drawn by NVGView

private:
	BaseController* ctrl;
};

class PolyView : public VSTGUI::CView {
public:
	PolyView(const VSTGUI::CRect& size, BaseController* c);
	~PolyView() override;

	VSTGUI::CMouseEventResult onMouseEntered(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseExited(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
	bool onWheel(const VSTGUI::CPoint& where, const VSTGUI::CMouseWheelAxis& axis, const float& distance,
	             const VSTGUI::CButtonState& buttons) override;
	void draw(VSTGUI::CDrawContext*) override {}  // drawn by NVGView

private:
	BaseController* ctrl;
	bool dragging = false, moved = false;
	double startY = 0.0;
	int startN = 0;
};

}}} // namespaces
