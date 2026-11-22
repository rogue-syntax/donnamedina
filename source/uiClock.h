#pragma once
// The editor's clock (2026-11-21). Every time-based animation in the editor (knob trails, glows,
// playheads, scope peak hold, logo effect, ...) reads the time here instead of steady_clock, so the
// film tool (video/film) can step it one video frame at a time. In the plugin nothing sets a manual
// time, so now() is always the real steady_clock time.

#include <chrono>

namespace uiClock {

using clock = std::chrono::steady_clock;

inline bool& manualOn() { static bool on = false; return on; }
inline clock::time_point& manualTime() { static clock::time_point t; return t; }

inline clock::time_point now() { return manualOn() ? manualTime() : clock::now(); }

// Film tool only: from now on the editor sees this time (seconds; any origin).
inline void setManualSeconds(double s) {
	manualTime() = clock::time_point(std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(s)));
	manualOn() = true;
}
inline void useRealTime() { manualOn() = false; }

} // namespace uiClock
