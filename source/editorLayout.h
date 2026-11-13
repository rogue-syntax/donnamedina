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

} // namespace editorLayout
