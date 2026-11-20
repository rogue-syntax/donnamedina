#pragma once
// Editor geometry shared by the controller, NVGViewController and NVGView.
//
// The editor window is kEditorWidth x kEditorHeight. The original panel art is 1148x542; it was
// moved by (kArtOffsetX, kArtOffsetY) = (-35, -15) when the window was widened to 1248 (2026-11-13).
// Positions measured on the original art (hard-coded in NVGView) are converted to window
// coordinates with artX()/artY(); the control positions in vstgui.uidesc are window coordinates.
//
// Images:
// - the panel image (resources/bkg1.png) was re-exported at the window size with the panels already
//   moved (2026-11-16, from fwrksrc/svg): drawn at kPanelImg* (0,0 1248x542);
// - the about image (dm_about_window.png) is still the original art: drawn at kAboutImg*.
// Keep image offsets multiples of 5 so the art's 5 px grid lines up with the procedural grid drawn
// around it (NVGView::drawBkg).

namespace editorLayout {

constexpr int kEditorWidth = 1248;
constexpr int kEditorHeight = 542;

constexpr int kArtWidth = 1148;  // original art size
constexpr int kArtHeight = 542;
constexpr int kArtOffsetX = -35;  // original art -> window
constexpr int kArtOffsetY = -15;

struct ImageRect { int x, y, w, h; };
constexpr ImageRect kPanelImg {0, 0, 1248, 542};                              // bkg1.png
constexpr ImageRect kAboutImg {kArtOffsetX, kArtOffsetY, kArtWidth, kArtHeight};  // dm_about_window.png
// Logo (dmlogo_screenres.png, 2026-11-18): top right, drawn after the rest of the UI (NVGView::drawLogo).
constexpr ImageRect kLogoImg {920, 6, 322, 72};

// About screen (2026-11-18), drawn in code by NVGView::drawMoreInfoWindow over a dark backdrop:
// a box centred in the window; the close box (#shutWin in vstgui.uidesc: origin 914,66 size 30x24)
// is its top-right corner of the title bar. The Rogue Syntax and VST logos are cut from
// dm_about_window.png (source rectangles in that image).
namespace about {
constexpr ImageRect kBox {304, 66, 640, 410};
constexpr float kTitleH = 24.0f;
constexpr ImageRect kClose {kBox.x + kBox.w - 30, kBox.y, 30, 24};
constexpr ImageRect kRogueSrc {292, 218, 342, 59};
constexpr ImageRect kVstSrc {696, 217, 163, 62};
// Links row (website, support mail) and info row (registration, format/build) above the footer.
// The links, the ABOUT / CREDITS tabs of the title bar and the credits page's wheel scrolling go
// through the invisible custom view "aboutLinks" (presetBarView.h; vstgui.uidesc origin
// kView.x,kView.y size kView.w x kView.h: the box above the footer, without the close box;
// mouse-enabled only while the about screen is open). Zone widths fit 11 px text (6.62 px/glyph).
constexpr ImageRect kView {kBox.x, kBox.y, kBox.w - 30, kBox.h - 30};   // 304,66 610x380
constexpr ImageRect kTabAbout {kBox.x + 8, kBox.y, 58, 24};
constexpr ImageRect kTabCredits {kBox.x + 70, kBox.y, 72, 24};
constexpr ImageRect kCreditsArea {kBox.x + 40, kBox.y + 36, kBox.w - 80, kBox.h - 76};  // scrolled text
constexpr float kPad = 40.0f;
constexpr float kInfoLineY = 384.0f;  // divider above the two rows
constexpr ImageRect kLinks {kBox.x + 40, 392, kBox.w - 80, 18};
constexpr ImageRect kWebZone {kLinks.x, kLinks.y, 136, 18};                     // WWW.ROGUE-SYNTAX.COM
constexpr ImageRect kMailZone {kLinks.x + kLinks.w - 150, kLinks.y, 150, 18};   // ADMIN@ROGUE-SYNTAX.COM
constexpr float kInfoY = 424.0f;  // middle of the info row
} // namespace about

// Registration screen (licensing v1, 2026-11-19): drawn by NVGView::drawBadReg over everything while
// the plugin has no valid license. The invisible custom view "licensePanel" (presetBarView.h,
// vstgui.uidesc origin kBox.x,kBox.y size kBox.w x kBox.h, last in the template so it is on top)
// takes the clicks; it is mouse-enabled only while unlicensed.
namespace licenseScreen {
constexpr ImageRect kBox {202, 75, 646, 341};      // the old box at art (237, 90)
constexpr float kCx = 525.0f;                      // text centre, art x 560
constexpr ImageRect kPaste {435, 296, 180, 30};    // "Paste license" button
constexpr float kMsgY = 348.0f;                    // result of the last paste
constexpr ImageRect kWeb {450, 374, 150, 18};      // WWW.ROGUE-SYNTAX.COM
} // namespace licenseScreen

// Convert a position in the original 1148x542 art to window coordinates.
constexpr float artX (float x) { return x + kArtOffsetX; }
constexpr float artY (float y) { return y + kArtOffsetY; }

// Background grid: 5 px cells, line colour on multiples of 5. Drawn live over the whole window
// under bkg1.png, which is transparent outside the panels (2026-11-16), so it shows wherever the
// art leaves space (NVGView::drawPanelBase).
constexpr int kGridStep = 5;

// Transparent display windows of bkg1.png (window coordinates): the three envelopes and the three
// LFOs, drawn before the art. drawPanelBase fills them dark so the grid stays out of the displays.
constexpr ImageRect kDisplayWin[6] {{15, 303, 138, 51}, {166, 303, 135, 51}, {314, 303, 135, 51},
                                    {462, 303, 135, 51}, {610, 303, 135, 51}, {758, 303, 138, 51}};

// STEP CTRL section (2026-11-17): drawn kStepDY lower than in bkg1.png and the SVG. NVGView::drawBkg
// draws the art rectangle kStepArt (step buttons, STEP CTRL frame, STEP CTRL TARGET label) shifted;
// the vstgui.uidesc origins of the step buttons, #stepCtrlTarg and the chorus knobs include the
// offset, and drawBlockFx offsets the step button outlines (blockMap row 2).
constexpr int kStepDY = 10;
constexpr ImageRect kStepArt {0, 455, 745, 65};

// Right column (window coordinates), drawn by NVGView::drawRightColumn over the grid.
namespace column {
// The column content moved 10 px left (2026-11-17, kDX) to make room for the frame's right edge and
// the brackets right of the boxes; the uidesc origins of the column controls include it.
constexpr float kDX = -10.0f;
constexpr float kX0 = 928.0f + kDX;
constexpr float kX1 = 1240.0f + kDX;
constexpr float kCenterX = (kX0 + kX1) * 0.5f;

struct Band { float y0, y1; constexpr float h () const { return y1 - y0; } };
// Everything below the title moved down kDY (2026-11-17) so the DELAY labels share a row with the
// STEP CTRL TARGET and CHORUS labels. The vstgui.uidesc origins of the column controls (presetBar,
// polyCount, master/play/delay knobs, HQ/VEL toggles) include it too.
constexpr float kDY = 16.0f;
constexpr Band kTitle {3.0f, 71.0f};
// PRESET: label at y0 + 2, the bar (presetBar::kBar) at y0 + 17 .. y1 - 3, like CONTROL and SCOPE.
constexpr Band kPreset {63.0f + kDY, 105.0f + kDY};
constexpr Band kReadout {111.0f + kDY, 175.0f + kDY};
constexpr Band kScope {181.0f + kDY, 305.0f + kDY};   // waveform over the sounding voices (voices band merged in)
constexpr Band kMaster {311.0f + kDY, 379.0f + kDY};
// Below the master row (2026-11-15; the logo band was here): PLAY and FX rows of four knob slots
// (slotCenterX), the knob tops at y0 + kRowKnobDY (30 px knobs), labels at y0 + kRowLabelDY.
constexpr Band kPlay {385.0f + kDY, 449.0f + kDY};
constexpr Band kFx {455.0f + kDY, 518.0f + kDY};
constexpr float kRowKnobDY = 17.0f, kRowLabelDY = 50.0f;
constexpr float kPlayLowDY = 9.0f;  // PLAY: DRIFT / EXPR / target labels (their knobs are 11 px lower than SNAP)
constexpr float kSlotX0 = 937.0f + kDX, kSlotW = 73.5f;
constexpr float slotCenterX (int i) { return kSlotX0 + kSlotW * (i + 0.5f); }

// Source rectangles in the art (art coordinates) for the redrawn title and logo.
struct Rect { float x, y, w, h; };
constexpr Rect kTitleArt {967.0f, 29.0f, 143.0f, 63.0f};

// Master row knobs, four equal slots (must match the origins/sizes of #masterVol, #panLR, #panLREnv,
// #panLREnvSrc in vstgui.uidesc).
constexpr Rect kVolKnob {954.0f + kDX, 328.0f + kDY, 40.0f, 40.0f};
constexpr Rect kPanKnob {1027.0f + kDX, 328.0f + kDY, 40.0f, 40.0f};
constexpr Rect kPanAmtKnob {1106.0f + kDX, 333.0f + kDY, 30.0f, 30.0f};
constexpr Rect kPanSrcKnob {1179.0f + kDX, 333.0f + kDY, 30.0f, 30.0f};
// Frame around MASTER, PLAY and DELAY (2026-11-17), stepping left at kChorusT to take in the CHORUS
// knobs of the main area (NVGView::drawColumnFrame). Its left edge continues down from kChorusT as
// the CHORUS | DELAY separator; PLAY and DELAY start at the divider lines kPlayLine / kFxLine. The
// four sections are hover blocks too (NVGView::drawBlockFx).
constexpr float kFrameL = kX0 - 4.0f;           // 914
constexpr float kFrameR = kX1 + 5.0f;           // 1235
constexpr float kFrameT = kMaster.y0 - 4.0f;    // 323
constexpr float kFrameB = 536.0f;
constexpr float kChorusL = 739.0f, kChorusT = 458.0f;
constexpr float kPlayLine = kPlay.y0 + 12.0f;   // 413
constexpr float kFxLine = kFx.y0 + 12.0f;       // 483
// The PLAY / DELAY dividers run at kTabH above the line from kFrameL to the end of the section label
// (kPlayTabX / kFxTabX: label x kX0 + 9, plus 6.6 px per 11 px glyph of the UI font (DejaVu Sans Mono, 0.602 em), plus 5), slant
// down 45 degrees to the line and run along it to kFrameR. The hover outlines of MASTER / PLAY /
// DELAY follow the same path.
constexpr float kTabH = 14.0f;
constexpr float kPlayTabX = kX0 + 9.0f + 4 * 6.6f + 5.0f;   // "PLAY"
constexpr float kFxTabX = kX0 + 9.0f + 5 * 6.6f + 5.0f;     // "DELAY"
// PRESET, CONTROL and SCOPE boxes (2026-11-18): the left edge lines up with the top of the frame's
// top-left chamfer, the right edge with the frame's right edge. Their labels start at kBoxL.
constexpr float kBoxL = kFrameL + 6.0f;        // 920
constexpr float kBoxR = kFrameR;               // 1235
// Brackets right of the boxes, outside the line.
constexpr float kBracketX = kBoxR + 7.0f;      // 1242
// #moreInfo (opens the about window) covers the logo (kLogoImg, 2026-11-18); the hover effect is
// drawn by NVGView::drawLogo.
constexpr Rect kMoreInfo {920.0f, 6.0f, 322.0f, 72.0f};
} // namespace column

} // namespace editorLayout
