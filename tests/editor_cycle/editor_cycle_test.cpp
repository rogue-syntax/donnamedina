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

// EDITOR_SHOT=<file.bmp>: open the editor once in an on-screen window sized from IPlugView::getSize,
// let it draw, and save the client area as a 24-bit BMP (for checking layout changes by eye).
int takeEditorShot(rs::DX10Controller* ctrl, const char* path) {
	IPlugView* view = ctrl->createView(ViewType::kEditor);
	if (!view) { std::printf("createView failed\n"); return 2; }
	ViewRect vr;
	view->getSize(&vr);
	const int w = vr.getWidth(), h = vr.getHeight();
	std::printf("editor size %d x %d\n", w, h);
	RECT r{0, 0, w, h};
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwnd = CreateWindowExW(WS_EX_TOPMOST, L"STATIC", L"DonnaMedina editor shot", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	                            0, 0, r.right - r.left, r.bottom - r.top, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	ShowWindow(hwnd, SW_SHOW);
	pumpFor(100);
	if (view->attached(hwnd, kPlatformTypeHWND) != kResultTrue) { std::printf("attached failed\n"); return 2; }
	// As a host does after attaching. VST3Editor::onSize ignores a size equal to the frame's own,
	// and without a host IPlugFrame the platform window was never sized, so go via another size.
	ViewRect tmp(0, 0, w - 1, h - 1);
	view->onSize(&tmp);
	view->onSize(&vr);
	// No processor here, so the registration overlay would hide the panel; drop it for the shot.
	if (ctrl->nvgViewController && ctrl->nvgViewController->nvgView)
		ctrl->nvgViewController->nvgView->badReg = false;
	pumpFor(1500);

	// Copy the client area from the screen (the window is topmost): PrintWindow misses the
	// plugin's OpenGL child window.
	SetForegroundWindow(hwnd);
	pumpFor(300);
	// The VSTGUI child windows come up 0x0 in this bare host (no IPlugFrame); size them directly.
	const SIZE editorSize{w, h};
	EnumChildWindows(hwnd, [](HWND c, LPARAM lp) -> BOOL {
		RECT cr{};
		GetWindowRect(c, &cr);
		if (cr.right - cr.left == 0) {
			const SIZE* sz = reinterpret_cast<const SIZE*>(lp);
			SetWindowPos(c, nullptr, 0, 0, sz->cx, sz->cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		return TRUE;
	}, reinterpret_cast<LPARAM>(&editorSize));
	pumpFor(500);
	EnumChildWindows(hwnd, [](HWND c, LPARAM) -> BOOL {
		char cls[128] = {};
		GetClassNameA(c, cls, sizeof(cls));
		RECT cr{};
		GetWindowRect(c, &cr);
		std::printf("  child %s  %ld,%ld %ldx%ld  visible %d\n", cls, cr.left, cr.top, cr.right - cr.left, cr.bottom - cr.top, IsWindowVisible(c));
		return TRUE;
	}, 0);
	POINT org{0, 0};
	ClientToScreen(hwnd, &org);
	// EDITOR_SHOT_SCOPE=1: no processor here, so hand the controller one scope frame the way the
	// processor does (PMSG_SCOPE_CHAR message with a binary "data" attribute): a 220 Hz tone with
	// some 3rd harmonic, starting at a rising zero crossing like a triggered capture.
	if (std::getenv("EDITOR_SHOT_SCOPE")) {
		std::vector<float> f(rs::BaseController::kScopeLength);
		for (size_t i = 0; i < f.size(); i++) {
			const double ph = 2.0 * 3.14159265358979 * 220.0 * double(i) / 44100.0;
			f[i] = static_cast<float>(0.55 * std::sin(ph) + 0.2 * std::sin(3.0 * ph));
		}
		HostMessage* m = new HostMessage;
		m->setMessageID("d");
		m->getAttributes()->setBinary("data", f.data(), static_cast<uint32>(f.size() * sizeof(float)));
		ctrl->notify(m);
		m->release();
		std::printf("scope frames received by the controller: %u\n", ctrl->scopeFrames);
		// and a voice-strip state: A minor chord held (one soft), two notes releasing
		// (EDITOR_SHOT_SCOPE=2: one held note instead, the narrowest range)
		unsigned char st[128] = {};
		if (std::atoi(std::getenv("EDITOR_SHOT_SCOPE")) == 2) {
			st[60] = 0x80 | 120;
		} else {
			st[57] = 0x80 | 127; st[60] = 0x80 | 110; st[64] = 0x80 | 40;
			st[45] = 70; st[69] = 15;
		}
		HostMessage* mv = new HostMessage;
		mv->setMessageID("f");
		mv->getAttributes()->setBinary("data", st, sizeof(st));
		ctrl->notify(mv);
		mv->release();
		// and an output peak: -4.6 dBFS for the chord, an over (+1.2 dB, CLIP) for the single note
		HostMessage* mp = new HostMessage;
		mp->setMessageID("g");
		mp->getAttributes()->setFloat("peak", std::atoi(std::getenv("EDITOR_SHOT_SCOPE")) == 2 ? 1.15 : 0.59);
		ctrl->notify(mp);
		mp->release();
		pumpFor(1000);  // the scope's pitch range glides to the new notes
	}
	// EDITOR_SHOT_LFO=1: LFO settings that show the display's cases: 1 square with a delay,
	// 2 saw synced to 1/8, 3 a fast sine after a long delay (drawn as a band).
	// EDITOR_SHOT_LFO=2: 1 triangle phase 0.25, 2 sine 1/4 dotted, 3 slow sine.
	if (const char* lf = std::getenv("EDITOR_SHOT_LFO")) {
		const bool two = std::atoi(lf) == 2;
		struct PV { int pid; double v; };
		const PV one[] = {
			{LFO_1_TYPE, 1.0}, {LFO_1_RATE, 0.1}, {LFO_1_AMP, 0.8}, {LFO_1_DELAY, 0.3}, {LFO_1_PHASE, 0.5}, {LFO_1_BPM, 0},
			{LFO_2_TYPE, 1.0 / 3}, {LFO_2_RATE, 0.5}, {LFO_2_AMP, 0.6}, {LFO_2_DELAY, 0}, {LFO_2_PHASE, 0.5}, {LFO_2_BPM, 1},
			{LFO_3_TYPE, 0}, {LFO_3_RATE, 0.9}, {LFO_3_AMP, 0.5}, {LFO_3_DELAY, 0.6}, {LFO_3_PHASE, 0.5}, {LFO_3_BPM, 0}};
		const PV alt[] = {
			{LFO_1_TYPE, 2.0 / 3}, {LFO_1_RATE, 0.2}, {LFO_1_AMP, 1.0}, {LFO_1_DELAY, 0}, {LFO_1_PHASE, 0.25}, {LFO_1_BPM, 0},
			{LFO_2_TYPE, 0}, {LFO_2_RATE, 0.3}, {LFO_2_AMP, 0.9}, {LFO_2_DELAY, 0.1}, {LFO_2_PHASE, 0.5}, {LFO_2_BPM, 1},
			{LFO_3_TYPE, 0}, {LFO_3_RATE, 0.0}, {LFO_3_AMP, 0.3}, {LFO_3_DELAY, 0}, {LFO_3_PHASE, 0.75}, {LFO_3_BPM, 0}};
		for (const PV& p : (two ? alt : one))
			ctrl->setParamNormalized(p.pid, p.v);
		pumpFor(300);
	}
	// EDITOR_SHOT_HOVER=<pid>: put the real cursor on that control (exercises the frame's mouse
	// observer -> NVGView::onMouseEntered -> showParamReadout), then let the readout animate in.
	if (const char* hv = std::getenv("EDITOR_SHOT_HOVER")) {
		const int pid = std::atoi(hv);
		rs::NVGView* nv = ctrl->nvgViewController ? ctrl->nvgViewController->nvgView : nullptr;
		if (nv && pid >= 0 && pid < 1000 && nv->controlMap[pid]) {
			const CRect cr = nv->controlMap[pid]->getViewSize();
			const int cx = org.x + static_cast<int>(cr.getCenter().x), cy = org.y + static_cast<int>(cr.getCenter().y);
			// Send the move to the VSTGUI frame window (the window under that point; the GL child is
			// disabled, so a real move goes there too). The real cursor is not touched.
			POINT pt{cx, cy};
			HWND under = WindowFromPoint(pt);
			char cls[128] = {};
			GetClassNameA(under, cls, sizeof(cls));
			std::printf("window at %d,%d: %s\n", cx, cy, cls);
			POINT cp = pt;
			ScreenToClient(under, &cp);
			SendMessage(under, WM_MOUSEMOVE, 0, MAKELPARAM(cp.x, cp.y));
			// The frame asks Windows for WM_MOUSELEAVE, which arrives at once because the real cursor
			// is elsewhere; drop it, as if the mouse stayed on the knob.
			const ULONGLONG end = GetTickCount64() + 1500;
			int leavesDropped = 0;
			do {
				MSG m;
				while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) {
					if (m.message == WM_MOUSELEAVE) {
						leavesDropped++;
						continue;
					}
					TranslateMessage(&m);
					DispatchMessage(&m);
				}
				Sleep(5);
			} while (GetTickCount64() < end);
			std::printf("WM_MOUSELEAVE dropped: %d\n", leavesDropped);
			std::printf("hover pid %d at %d,%d: hoverPid %d, name \"%s\" (shown \"%s\", alpha %.2f), value \"%s\" (shown \"%s\", alpha %.2f)\n", pid,
			            static_cast<int>(cr.getCenter().x), static_cast<int>(cr.getCenter().y), nv->hoverPid,
			            nv->targDisplayTxt.selectedCtrlTarg.c_str(), nv->targDisplayTxt.selectedCtrlTargBuilder.c_str(),
			            nv->targDisplayTxt.textDisplayDecay,
			            nv->targValueTxt.selectedCtrlTarg.c_str(), nv->targValueTxt.selectedCtrlTargBuilder.c_str(),
			            nv->targValueTxt.textDisplayDecay);
		}
		else {
			std::printf("hover pid %d: no control\n", pid);
		}
	}
	HDC wdc = GetDC(nullptr);
	HDC mdc = CreateCompatibleDC(wdc);
	HBITMAP bmp = CreateCompatibleBitmap(wdc, w, h);
	HGDIOBJ old = SelectObject(mdc, bmp);
	BitBlt(mdc, 0, 0, w, h, wdc, org.x, org.y, SRCCOPY);
	SelectObject(mdc, old);

	BITMAPINFOHEADER bi{};
	bi.biSize = sizeof(bi);
	bi.biWidth = w;
	bi.biHeight = h;  // bottom-up, as BMP files store it
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	const int stride = ((w * 3 + 3) / 4) * 4;
	std::vector<unsigned char> px(static_cast<size_t>(stride) * h);
	GetDIBits(mdc, bmp, 0, h, px.data(), reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
	BITMAPFILEHEADER bf{};
	bf.bfType = 0x4D42;
	bf.bfOffBits = sizeof(bf) + sizeof(bi);
	bf.bfSize = bf.bfOffBits + static_cast<DWORD>(px.size());
	if (FILE* f = std::fopen(path, "wb")) {
		std::fwrite(&bf, sizeof(bf), 1, f);
		std::fwrite(&bi, sizeof(bi), 1, f);
		std::fwrite(px.data(), 1, px.size(), f);
		std::fclose(f);
		std::printf("saved %s\n", path);
	}
	DeleteObject(bmp);
	DeleteDC(mdc);
	ReleaseDC(nullptr, wdc);
	view->removed();
	view->release();
	DestroyWindow(hwnd);
	return 0;
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

	// READOUT_TABLE=1: the readout text (DX10Controller::readoutValue) for a set of params.
	if (std::getenv("READOUT_TABLE")) {
		const int pids[] = {WIDTH_, TUNE_, FINE_, OCT_, INT_, MOD_OCT_, MOD_INT_, MOD_BEAT_, INT_ENV, OCT_ENV,
		                    TUNE_ENV, GAIN_, FILTER_, FILTER_Q, MOD_GAIN_, MOD_OSC_GAIN, PAN_LR,
		                    ENV_1_ATTACK, ENV_1_DECAY, ENV_1_RELEASE, LFO_1_RATE, LFO_1_AMP, LFO_1_DELAY, LFO_1_PHASE};
		const double vals[] = {0.0, 0.25, 0.5, 0.75, 1.0};
		for (int pid : pids) {
			std::printf("%-18s", paramDisplay::label(pid));
			for (double v : vals)
				std::printf(" | %-22s", ctrl->readoutValue(pid, v).c_str());
			std::printf("\n");
		}
		// LFO 1 synced, and an env amount with a source set
		ctrl->setParamNormalized(LFO_1_BPM, 1.0);
		std::printf("%-18s", "LFO 1 RATE (sync)");
		for (double v : vals)
			std::printf(" | %-22s", ctrl->readoutValue(LFO_1_RATE, v).c_str());
		std::printf("\n");
		ctrl->setParamNormalized(LFO_1_BPM, 0.0);
		ctrl->setParamNormalized(FILTER_ENV_SRC, 4.0 / 6.0);
		std::printf("%-18s", "CUTOFF MOD (src)");
		for (double v : vals)
			std::printf(" | %-22s", ctrl->readoutValue(FILTER_ENV, v).c_str());
		std::printf("\n");
		return 0;
	}

	if (const char* shot = std::getenv("EDITOR_SHOT"))
		return takeEditorShot(ctrl, shot);

	// B-59: opening the editor replays the reset buttons' values, which re-centred WIDTH/TUNE/FINE
	// (and the MOD ones) with performEdit. Off-centre values must survive the open, and the preset
	// bar must not show the sound as edited.
	int b59fail = 0;
	ctrl->setParamNormalized(TUNE_, 0.8);
	ctrl->setParamNormalized(MOD_TUNE_, 0.75);
	ctrl->setParamNormalized(MOD_WIDTH_, 0.3);
	ctrl->presetDirty = false;

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
		if (c == 1) {
			const double t = ctrl->getParamNormalized(TUNE_), mt = ctrl->getParamNormalized(MOD_TUNE_),
			             mw = ctrl->getParamNormalized(MOD_WIDTH_);
			const bool ok = std::fabs(t - 0.8) < 1e-9 && std::fabs(mt - 0.75) < 1e-9 && std::fabs(mw - 0.3) < 1e-9 &&
			                !ctrl->presetDirty;
			if (!ok) b59fail = 1;
			std::printf("B-59 editor open keeps TUNE_ %.3f MOD_TUNE_ %.3f MOD_WIDTH_ %.3f, edited flag %d  %s\n", t, mt, mw,
			            int(ctrl->presetDirty), ok ? "PASS" : "FAIL");
		}
		if (c == 1 && std::getenv("NO_GUI_HANDLER_PROBE") == nullptr) {  // set it to skip the probe
			// try/catch review (B-33): GUI handlers called with ids that have no name / no view.
			// The old code relied on try/catch here, which cannot catch null or out-of-range access.
			std::printf("  GUI handlers with unknown ids (editor open)... ");
			rs::NVGView* nv = ctrl->nvgViewController ? ctrl->nvgViewController->nvgView : nullptr;
			if (nv) {
				nv->onStepCtrlSwitch(153);       // param without a view (the "pid 153" comment)
				nv->onEnvLev(999, "x");          // no name in envNameMap
				nv->onEnvBipolar(170, "x");
				nv->onOnOffBtn(960, 1);
				nv->onEnvSrc(WIDTH_ENV_SRC, 42); // env source index beyond the 7 names
				nv->onResetButton(5);            // not a reset button
				nv->onStepCtrlButton(153);
				nv->onToggleButton(153);
				nv->onEnv(80, "x");              // beyond the generator param names
				pumpFor(300);                    // draw frames with those animations queued
				std::printf("survived\n");
			}
			else {
				std::printf("no view (skipped)\n");
			}
		}
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
	if (track::nLive != 0 || dangling || mallocPerCycle > 0.05 || b11fail || b59fail) result = 1;

	std::printf("malloc growth %.4f MB/cycle (limit 0.05)  %s\n", mallocPerCycle, mallocPerCycle > 0.05 ? "FAIL" : "PASS");
	std::printf("\n%s\n", result == 0 ? "0 failure(s)" : "FAILED");

	DestroyWindow(hwnd);
	ctrl->terminate();
	ctrl->release();
	return result;
}
