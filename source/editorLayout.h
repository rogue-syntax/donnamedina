#pragma once
// Editor geometry shared by the controller, NVGViewController and NVGView.
//
// The editor window is kEditorWidth x kEditorHeight. The painted panel art (resources/bkg1.png and
// dm_about_window.png, both 1148x542) is drawn at (kArtOffsetX, kArtOffsetY), so the art can be moved
// without re-exporting it. The control positions in vstgui.uidesc are in window coordinates and were
// shifted by the same offset when it was introduced (2026-11-13: -35,-15, window widened to 1248).
// Coordinates written for the art (hard-coded positions in NVGView) are converted with artX()/artY().
//
// If the background art is re-exported at the window size with the panels already moved, set the
// offsets to 0 and kArtWidth/kArtHeight to the new image size. Keep the offsets multiples of 5 so the
// art's 5 px grid lines up with the procedural grid drawn around it (NVGView::drawBkg).

namespace editorLayout {

constexpr int kEditorWidth = 1248;
constexpr int kEditorHeight = 542;

constexpr int kArtWidth = 1148;
constexpr int kArtHeight = 542;
constexpr int kArtOffsetX = -35;
constexpr int kArtOffsetY = -15;

// Convert a position in the original 1148x542 art to window coordinates.
constexpr float artX (float x) { return x + kArtOffsetX; }
constexpr float artY (float y) { return y + kArtOffsetY; }

// Background grid of the art: 5 px cells, line colour on multiples of 5.
constexpr int kGridStep = 5;

// Right column (window coordinates), drawn by NVGView::drawRightColumn. The art's own right column
// (title, pan, control, build info, logo boxes) is covered with grid from kBlankArtX0 (art x) rightwards;
// the title and logo are redrawn from the art at the positions below.
namespace column {
constexpr float kBlankArtX0 = 950.0f;
constexpr float kX0 = 928.0f;
constexpr float kX1 = 1240.0f;
constexpr float kCenterX = (kX0 + kX1) * 0.5f;

struct Band { float y0, y1; constexpr float h () const { return y1 - y0; } };
constexpr Band kTitle {3.0f, 71.0f};
constexpr Band kPreset {77.0f, 105.0f};
constexpr Band kReadout {111.0f, 175.0f};
constexpr Band kScope {181.0f, 277.0f};
constexpr Band kMaster {283.0f, 351.0f};
constexpr Band kVoices {357.0f, 379.0f};
constexpr Band kLogo {385.0f, 518.0f};

// Source rectangles in the art (art coordinates) for the redrawn title and logo.
struct Rect { float x, y, w, h; };
constexpr Rect kTitleArt {967.0f, 29.0f, 143.0f, 63.0f};
constexpr Rect kLogoArt {972.0f, 389.0f, 133.0f, 124.0f};

// Master row knobs, four equal slots (must match the origins/sizes of #masterVol, #panLR, #panLREnv,
// #panLREnvSrc in vstgui.uidesc).
constexpr Rect kVolKnob {954.0f, 300.0f, 40.0f, 40.0f};
constexpr Rect kPanKnob {1027.0f, 300.0f, 40.0f, 40.0f};
constexpr Rect kPanAmtKnob {1106.0f, 305.0f, 30.0f, 30.0f};
constexpr Rect kPanSrcKnob {1179.0f, 305.0f, 30.0f, 30.0f};
// #moreInfo (opens the about window) sits on the logo.
constexpr Rect kMoreInfo {kCenterX - 70.0f, 387.0f, 140.0f, 129.0f};
} // namespace column

} // namespace editorLayout
