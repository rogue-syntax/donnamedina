// Editor open/close cycle test for Donna Medina (B-28, B-29, "+30 MB per window open").
//
// Opens the real editor (DX10Controller::createView("editor") attached to a real window, so the
// NanoVG/OpenGL view is created and drawn) and closes it again, N times. After each cycle it
// records the process's private memory and how many NVGView objects are still alive.
//
// Build + run: tests\editor_cycle\build_and_run.bat (after building Release|x64 of the plugin).
// A window appears briefly (off-screen position) while the test runs.

#include <windows.h>
#include <psapi.h>
#include "../../source/mdaDX10Controller.h"
#include "../../source/NVGView.h"  // NVGViewController comes in via mdaDX10Controller.h (no include guard)
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/gui/iplugview.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <new>
#include <vector>

// Normally defined by the SDK's dllmain.cpp; VSTGUI loads the UI description and images from this
// module's resources (the plugin's .res is linked into the exe).
void* moduleHandle = nullptr;

using namespace Steinberg;
using namespace Steinberg::Vst;

// ---- count live NVGView objects: track allocations of exactly sizeof(NVGView) ----------------
namespace track {
const size_t kSize = sizeof(rs::NVGView);
void* live[256];
int nLive = 0;
int created = 0;
long long liveBytes = 0;  // all operator new bytes not yet deleted
// NVGViewController (B-29): the controller keeps a raw pointer to it
const size_t kCtlSize = sizeof(rs::NVGViewController);
void* liveCtl[256];
int nLiveCtl = 0;
void onNew(void* p, size_t n) {
	if (p) liveBytes += static_cast<long long>(_msize(p));
	if (p && n == kCtlSize && nLiveCtl < 256) liveCtl[nLiveCtl++] = p;
	if (n != kSize || !p) return;
	created++;
	if (nLive < 256) live[nLive++] = p;
}
bool isLiveCtl(void* p) {
	for (int i = 0; i < nLiveCtl; i++)
		if (liveCtl[i] == p) return true;
	return false;
}
void onDelete(void* p) {
	liveBytes -= static_cast<long long>(_msize(p));
	for (int i = 0; i < nLiveCtl; i++)
		if (liveCtl[i] == p) { liveCtl[i] = liveCtl[--nLiveCtl]; break; }
	for (int i = 0; i < nLive; i++)
		if (live[i] == p) { live[i] = live[--nLive]; return; }
}
}  // namespace track

void* operator new(size_t n) {
	void* p = std::malloc(n ? n : 1);
	if (!p) throw std::bad_alloc();
	track::onNew(p, n);
	return p;
}
void* operator new[](size_t n) { return operator new(n); }
void* operator new(size_t n, const std::nothrow_t&) noexcept {
	void* p = std::malloc(n ? n : 1);
	track::onNew(p, n);
	return p;
}
void* operator new[](size_t n, const std::nothrow_t& t) noexcept { return operator new(n, t); }
void operator delete(void* p) noexcept { if (p) { track::onDelete(p); std::free(p); } }
void operator delete[](void* p) noexcept { operator delete(p); }
void operator delete(void* p, size_t) noexcept { operator delete(p); }
void operator delete[](void* p, size_t) noexcept { operator delete(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { operator delete(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { operator delete(p); }

namespace {
void pumpFor(int ms) {
	const ULONGLONG end = GetTickCount64() + ms;
	do {
		MSG m;
		while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&m);
			DispatchMessage(&m);
		}
		Sleep(5);
	} while (GetTickCount64() < end);
}

// In-use bytes on the process heap (UCRT malloc and our operator new both allocate there).
double processHeapBusyMB() {
	HANDLE h = GetProcessHeap();
	HeapLock(h);
	PROCESS_HEAP_ENTRY e{};
	unsigned long long busy = 0;
	while (HeapWalk(h, &e))
		if (e.wFlags & PROCESS_HEAP_ENTRY_BUSY) busy += e.cbData;
	HeapUnlock(h);
	return busy / (1024.0 * 1024.0);
}

// Snapshot of busy process-heap blocks (address -> size), to find blocks that a cycle leaves behind.
struct Block { void* p; unsigned long long n; };
std::vector<Block> heapSnapshot() {
	std::vector<Block> v;
	v.reserve(200000);
	HANDLE h = GetProcessHeap();
	HeapLock(h);
	PROCESS_HEAP_ENTRY e{};
	while (HeapWalk(h, &e))
		if (e.wFlags & PROCESS_HEAP_ENTRY_BUSY) v.push_back({e.lpData, e.cbData});
	HeapUnlock(h);
	return v;
}

double privateMB() {
	PROCESS_MEMORY_COUNTERS_EX pmc{};
	pmc.cb = sizeof(pmc);
	GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
	return pmc.PrivateUsage / (1024.0 * 1024.0);
}
}  // namespace

int main(int argc, char** argv) {
	const int cycles = argc > 1 ? std::atoi(argv[1]) : 20;
	const bool verbose = argc > 2;
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	moduleHandle = GetModuleHandle(nullptr);
	// A host has COM initialised on its UI thread; VSTGUI decodes PNGs through WIC (COM).
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	std::printf("init controller\n");

	static HostApplication host;
	auto* ctrl = new rs::DX10Controller();
	if (ctrl->initialize(&host) != kResultTrue) {
		std::printf("controller initialize failed\n");
		return 2;
	}

	// B-11: this exe never constructs a processor (like a host that runs the controller on its
	// own). A preset change must still apply the real factory values. Preset 2 "OSC_" is 0.90 and
	// "MOD_OSC_" 0.6 in DX10Processor's preset table.
	int b11fail = 0;
	{
		ctrl->setParamNormalized(rs::BaseController::kPresetParam, (2 + 0.5) / rs::DX10Processor::kNumPrograms);
		const double osc = ctrl->getParamNormalized(OSC_), mosc = ctrl->getParamNormalized(MOD_OSC_);
		const bool ok = std::fabs(osc - 0.90) < 1e-6 && std::fabs(mosc - 0.6) < 1e-6;
		if (!ok) b11fail = 1;
		std::printf("B-11 preset 2 with no processor constructed: OSC_ %.3f (want 0.900), MOD_OSC_ %.3f (want 0.600)  %s\n",
		            osc, mosc, ok ? "PASS" : "FAIL");
	}

	HWND hwnd = CreateWindowExW(0, L"STATIC", L"DonnaMedina editor cycle test", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	                            -3000, 0, 1200, 600, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	ShowWindow(hwnd, SW_SHOWNOACTIVATE);
	pumpFor(100);

	std::printf("sizeof(NVGView) = %zu bytes\n", track::kSize);
	std::printf("cycle  private MB  delta   C++ heap live MB  delta   NVGView created/alive\n");
	const double start = privateMB();
	double prev = start;
	double prevHeap = track::liveBytes / (1024.0 * 1024.0);
	std::vector<double> after, afterHeap;
	std::vector<Block> snapA;  // after cycle 5
	double mallocAfterFirst = 0.0;
	for (int c = 1; c <= cycles; c++) {
		if (verbose) std::printf("  createView\n");
		IPlugView* view = ctrl->createView(ViewType::kEditor);
		if (!view) { std::printf("createView failed\n"); return 2; }
		if (verbose) std::printf("  attached\n");
		if (view->attached(hwnd, kPlatformTypeHWND) != kResultTrue) { std::printf("attached failed\n"); return 2; }
		if (verbose) std::printf("  pump\n");
		pumpFor(400);  // GL view created, animation timer draws frames
		if (verbose) std::printf("  removed\n");
		view->removed();
		if (verbose) std::printf("  release\n");
		view->release();
		if (verbose) std::printf("  pump\n");
		pumpFor(150);
		const double now = privateMB();
		const double heap = track::liveBytes / (1024.0 * 1024.0);
		after.push_back(now);
		afterHeap.push_back(heap);
		DWORD handles = 0;
		GetProcessHandleCount(GetCurrentProcess(), &handles);
		const double mallocMB = processHeapBusyMB() - heap;  // process heap minus operator new = malloc etc.
		if (c == 1) mallocAfterFirst = mallocMB;
		std::printf("%5d  %9.1f  %+6.1f   %13.3f  %+7.3f   %d / %d   handles %lu gdi %lu user %lu   malloc MB %.3f\n", c,
		            now, now - prev, heap, heap - prevHeap, track::created, track::nLive, handles,
		            GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS), GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS),
		            mallocMB);
		prev = now;
		prevHeap = heap;
		if (c == 5) snapA = heapSnapshot();
		if (c == 10 && !snapA.empty()) {
			// blocks alive after cycle 10 that did not exist after cycle 5 = left behind by cycles 6-10
			auto snapB = heapSnapshot();
			std::sort(snapA.begin(), snapA.end(), [](const Block& a, const Block& b) { return a.p < b.p; });
			std::vector<std::pair<unsigned long long, int>> bySize;  // size -> count
			unsigned long long total = 0;
			for (const auto& b : snapB) {
				auto it = std::lower_bound(snapA.begin(), snapA.end(), b,
				                           [](const Block& x, const Block& y) { return x.p < y.p; });
				if (it != snapA.end() && it->p == b.p && it->n == b.n) continue;
				total += b.n;
				auto s = std::find_if(bySize.begin(), bySize.end(), [&](const auto& q) { return q.first == b.n; });
				if (s == bySize.end()) bySize.push_back({b.n, 1}); else s->second++;
			}
			std::sort(bySize.begin(), bySize.end(),
			          [](const auto& a, const auto& b) { return a.first * a.second > b.first * b.second; });
			std::printf("  heap blocks new between cycle 5 and 10: %.3f MB in total; largest groups (size x count):\n",
			            total / (1024.0 * 1024.0));
			for (size_t i = 0; i < bySize.size() && i < 12; i++)
				std::printf("    %10llu bytes x %d\n", bySize[i].first, bySize[i].second);
			std::vector<Block>().swap(snapA);  // release the snapshot so it does not count as growth
			std::vector<Block>().swap(snapB);
			prevHeap = track::liveBytes / (1024.0 * 1024.0);
		}
	}
	// growth per cycle, ignoring the first cycle (one-time caches: fonts, driver, VSTGUI)
	const double perCycle = cycles > 2 ? (after.back() - after[0]) / (cycles - 1) : 0.0;
	const double heapPerCycle = cycles > 2 ? (afterHeap.back() - afterHeap[0]) / (cycles - 1) : 0.0;
	const double mallocPerCycle =
	    cycles > 2 ? ((processHeapBusyMB() - track::liveBytes / (1024.0 * 1024.0)) - mallocAfterFirst) / (cycles - 1) : 0.0;
	std::printf("\nmalloc (process heap minus operator new): %.4f MB per open/close after the first\n", mallocPerCycle);
	std::printf("\nprivate memory: start %.1f MB, after cycle 1 %.1f MB, end %.1f MB -> %.2f MB per open/close after the first\n",
	            start, after[0], after.back(), perCycle);
	std::printf("C++ heap (operator new) still allocated: %.3f MB per open/close after the first\n", heapPerCycle);
	std::printf("NVGView objects still alive after %d closes: %d\n", cycles, track::nLive);
	// B-29: after the last close, does DX10Controller still hold a pointer to a freed NVGViewController?
	void* held = ctrl->nvgViewController;
	const bool dangling = held && !track::isLiveCtl(held);
	std::printf("DX10Controller::nvgViewController after close: %s (NVGViewController objects alive: %d)  %s\n",
	            held ? (dangling ? "points to a FREED object" : "points to a live object") : "nullptr",
	            track::nLiveCtl, dangling ? "FAIL (dangling)" : "PASS");
	int result = 0;
	if (track::nLive != 0 || dangling || mallocPerCycle > 0.05 || b11fail) result = 1;

	std::printf("malloc growth %.4f MB/cycle (limit 0.05)  %s\n", mallocPerCycle, mallocPerCycle > 0.05 ? "FAIL" : "PASS");
	std::printf("\n%s\n", result == 0 ? "0 failure(s)" : "FAILED");

	DestroyWindow(hwnd);
	ctrl->terminate();
	ctrl->release();
	return result;
}
