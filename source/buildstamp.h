#pragma once
// Build stamp (branch, short commit, build time), e.g. "simd-voices 448d975  2026-11-19 11:05".
// Defined in source/buildstamp.cpp, which tools/buildstamp.ps1 writes at every build (pre-build
// step; the file is not in git). Shown in the editor's About window under the version.
extern const char* const kBuildStamp;
