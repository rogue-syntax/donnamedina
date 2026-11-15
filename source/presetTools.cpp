#include "presetTools.h"
#include "constants.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwctype>
#include <cstdio>
#include <random>

#include <windows.h>
#include <shlobj.h>

namespace presetTools {

namespace {

const uint32_t kStateMagic = 9999999;  // BaseController::kMagicNumber

struct Reader {
	const unsigned char* p;
	size_t left;
	bool u32(uint32_t& v) {
		if (left < 4) return false;
		std::memcpy(&v, p, 4);
		p += 4; left -= 4;
		return true;
	}
	bool f64(double& v) {
		if (left < 8) return false;
		std::memcpy(&v, p, 8);
		p += 8; left -= 8;
		return true;
	}
};

void putU32(std::vector<char>& b, uint32_t v) {
	const size_t n = b.size();
	b.resize(n + 4);
	std::memcpy(&b[n], &v, 4);
}
void putF64(std::vector<char>& b, double v) {
	const size_t n = b.size();
	b.resize(n + 8);
	std::memcpy(&b[n], &v, 8);
}

// ---- randomizer rules ----
// What each parameter may do. Ids not listed are kept.
enum class Rule {
	kKeep,     // never changed (carrier pitch, master volume, buttons, unknown)
	kFree,     // anywhere in 0..1 (stepped: any step)
	kRange,    // inside lo..hi (keeps the patch audible: gain, filter, amp envelope, pan)
	kEnvAmt,   // bipolar modulation amount, 0.5 = none: full random leaves most of them at none
	kRatio,    // modulator tune: harmonic ratios only in full random (x0.5, x1, x1.5, x2)
};
struct RuleInfo { Rule rule; double lo, hi, nudge; };

RuleInfo ruleFor(int pid) {
	const RuleInfo keep {Rule::kKeep, 0, 1, 0};
	const RuleInfo freeR {Rule::kFree, 0, 1, 0.10};
	auto range = [](double lo, double hi, double nudge) { return RuleInfo {Rule::kRange, lo, hi, nudge}; };
	if (pid < PARTPARAMS) {
		switch (pid) {
			case OSC_: case MOD_OSC_: case MOD_TYPE_:
			case WIDTH_: case MOD_WIDTH_:
			case MOD_OCT_: case MOD_INT_:
			case MOD_OSC_GAIN:
				return freeR;
			case WIDTH_ENV_SRC: case TUNE_ENV_SRC: case FINE_ENV_SRC: case OCT_ENV_SRC: case INT_ENV_SRC:
			case MOD_GAIN_ENV_SRC: case MOD_WIDTH_ENV_SRC: case MOD_TUNE_ENV_SRC: case MOD_FINE_ENV_SRC:
			case MOD_OCT_ENV_SRC: case MOD_INT_ENV_SRC: case MOD_BEAT_ENV_SRC: case GAIN_ENV_SRC:
			case FILTER_ENV_SRC: case PAN_LR_ENV_SRC:
				return freeR;
			case WIDTH_ENV: case MOD_GAIN_ENV: case MOD_WIDTH_ENV: case MOD_FINE_ENV:
			case GAIN_ENV: case FILTER_ENV: case PAN_LR_ENV:
				return {Rule::kEnvAmt, 0, 1, 0.08};
			case MOD_TUNE_ENV:
				return {Rule::kEnvAmt, 0.35, 0.65, 0.04};
			case MOD_TUNE_:
				return {Rule::kRatio, 0, 1, 0};
			case MOD_FINE_:
				return range(0.4, 0.6, 0.03);
			case MOD_GAIN_:
				return range(0.0, 0.8, 0.08);
			case GAIN_:
				return range(0.55, 0.9, 0.06);
			case FILTER_:
				return range(0.45, 1.0, 0.08);
			case FILTER_Q:
				return range(0.0, 0.6, 0.08);
			case PAN_LR:
				return range(0.35, 0.65, 0.05);
			default:
				// TUNE_/FINE_/OCT_/INT_ and their env amounts (carrier pitch), MOD_OCT_ENV,
				// MOD_INT_ENV, MOD_BEAT_*, AUX_*, SYNC_GEN_*: keep
				return keep;
		}
	}
	if (pid >= ENV_1_ATTACK && pid <= ENV_3_DELAY) {
		const int env = (pid - ENV_1_ATTACK) / 5, field = (pid - ENV_1_ATTACK) % 5;
		if (env == 0) {  // ENV 1 is the amp envelope: keep it playable
			if (field == 0) return range(0.0, 0.45, 0.06);   // attack
			if (field == 1) return range(0.8, 1.0, 0.04);    // decay: lower decays to silence within ~0.15 s
			                                                   // (factory presets use 0.86..1.0)
			if (field == 2) return range(0.1, 0.6, 0.06);    // release
			return keep;                                       // decay toggle, delay
		}
		if (field <= 3) return freeR;  // attack, decay, release, decay toggle
		return keep;                   // delay
	}
	if (pid >= LFO_1_RATE && pid <= LFO_3_DELAY) {
		const int field = (pid - LFO_1_RATE) % 5;
		return field <= 3 ? freeR : keep;  // rate, amp, type, bpm sync; delay kept
	}
	if (pid >= LFO_1_PHASE && pid <= LFO_3_PHASE)
		return freeR;
	if (pid >= WIDTH_ENV_ADD_MULT && pid <= MAIN_FILTER_ENV_ADD_MULT)
		return freeR;
	return keep;  // STEP_CTRL_TARG, MASTER_VOL, POLY_COUNT, reset buttons, view toggles, step buttons, ...
}

double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

} // namespace

bool parseState(const void* data, size_t size, State& out) {
	if (!data) return false;
	Reader r {static_cast<const unsigned char*>(data), size};
	State s;
	uint32_t first = 0;
	if (!r.u32(first)) return false;
	uint32_t count = first;
	if (first == kStateMagic) {
		s.hasHeader = true;
		if (!r.u32(s.program) || !r.u32(count)) return false;
	}
	else {
		s.hasHeader = false;
	}
	if (count > 100000 || size_t(count) * 8 > r.left) return false;
	s.values.resize(count);
	for (uint32_t i = 0; i < count; i++)
		r.f64(s.values[i]);
	if (r.u32(s.bypass)) {
		for (int k = 0; k < 32; k++) {  // tagged blocks, any order
			uint32_t tag = 0;
			double v = 0.0;
			if (!r.u32(tag) || !r.f64(v))
				break;
			if (tag == uint32_t(MASTER_VOL_STATE_TAG)) { s.hasMasterVol = true; s.masterVol = v; }
			else if (tag == uint32_t(POLY_STATE_TAG)) { s.hasPoly = true; s.poly = v; }
			else if (tag == uint32_t(STATE_VERSION_TAG) && v >= 1.0 && v < 1.0e6) s.version = uint32_t(v);
		}
	}
	out = std::move(s);
	return true;
}

std::vector<char> writeState(const State& s) {
	std::vector<char> b;
	b.reserve(32 + s.values.size() * 8);
	if (s.hasHeader) {
		putU32(b, kStateMagic);
		putU32(b, s.program);
	}
	putU32(b, uint32_t(s.values.size()));
	for (double v : s.values)
		putF64(b, v);
	putU32(b, s.bypass);
	if (s.hasMasterVol) {
		putU32(b, uint32_t(MASTER_VOL_STATE_TAG));
		putF64(b, s.masterVol);
	}
	if (s.hasPoly) {
		putU32(b, uint32_t(POLY_STATE_TAG));
		putF64(b, s.poly);
	}
	if (s.version > 0) {
		putU32(b, uint32_t(STATE_VERSION_TAG));
		putF64(b, double(s.version));
	}
	return b;
}

void randomize(State& s, const int* stepCounts, size_t count, Mode mode, uint32_t seed) {
	std::mt19937 rng(seed);
	std::uniform_real_distribution<double> u01(0.0, 1.0);
	const size_t n = (std::min)(count, s.values.size());
	for (size_t pid = 0; pid < n; pid++) {
		const int steps = stepCounts[pid];
		if (steps < 0)
			continue;
		const RuleInfo ri = ruleFor(int(pid));
		if (ri.rule == Rule::kKeep)
			continue;
		double& v = s.values[pid];
		if (!std::isfinite(v)) v = 0.5;

		if (mode == Mode::kVariation) {
			// switches and ratios stay; continuous values move a little; an unused modulation stays unused
			if (steps > 0 || ri.rule == Rule::kRatio || ri.nudge <= 0.0)
				continue;
			if (ri.rule == Rule::kEnvAmt && std::fabs(v - 0.5) < 1e-9)
				continue;
			// never push a value out of the safe range, but do not pull one in that already is outside
			const double lo = (std::min)(ri.lo, v), hi = (std::max)(ri.hi, v);
			v = (std::min)(hi, (std::max)(lo, v + (u01(rng) * 2.0 - 1.0) * ri.nudge));
			continue;
		}

		switch (ri.rule) {
			case Rule::kRatio: {
				static const double kTunes[] = {0.25, 0.5, 0.5, 0.75, 1.0};  // x0.5, x1 (twice), x1.5, x2
				v = kTunes[std::uniform_int_distribution<int>(0, 4)(rng)];
				break;
			}
			case Rule::kEnvAmt:
				v = u01(rng) < 0.65 ? 0.5 : ri.lo + (ri.hi - ri.lo) * u01(rng);
				break;
			case Rule::kRange:
			case Rule::kFree:
			default:
				if (steps > 0) {
					v = double(std::uniform_int_distribution<int>(0, steps)(rng)) / double(steps);
					v = (std::min)(ri.hi, (std::max)(ri.lo, v));
				}
				else {
					v = ri.lo + (ri.hi - ri.lo) * u01(rng);
				}
				break;
		}
		v = clamp01(v);
	}
}

std::string toUtf8(const std::wstring& w) {
	if (w.empty()) return {};
	const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), nullptr, 0, nullptr, nullptr);
	std::string s(size_t(n > 0 ? n : 0), '\0');
	if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), &s[0], n, nullptr, nullptr);
	return s;
}

std::wstring fromUtf8(const std::string& s) {
	if (s.empty()) return {};
	const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), nullptr, 0);
	std::wstring w(size_t(n > 0 ? n : 0), L'\0');
	if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), &w[0], n);
	return w;
}

std::wstring userPresetDir() {
	std::wstring docs;
	PWSTR p = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &p)) && p)
		docs = p;
	if (p) CoTaskMemFree(p);
	if (docs.empty())
		return {};
	return docs + L"\\VST3 Presets\\Rogue Syntax\\Donna Medina";
}

std::wstring parentDir(const std::wstring& path) {
	const size_t i = path.find_last_of(L"\\/");
	return i == std::wstring::npos ? std::wstring() : path.substr(0, i);
}

static std::wstring lowerW(std::wstring s) {
	for (auto& c : s) c = wchar_t(std::towlower(c));
	return s;
}

static std::wstring fileName(const std::wstring& path) {
	const size_t i = path.find_last_of(L"\\/");
	return i == std::wstring::npos ? path : path.substr(i + 1);
}

static std::wstring stem(const std::wstring& path) {
	const std::wstring f = fileName(path);
	const size_t d = f.find_last_of(L'.');
	return (d == std::wstring::npos || d == 0) ? f : f.substr(0, d);
}

bool hasExtension(const std::wstring& path, const wchar_t* ext) {
	const std::wstring e = lowerW(ext), p = lowerW(path);
	return p.size() >= e.size() && p.compare(p.size() - e.size(), e.size(), e) == 0;
}

bool ensureDir(const std::wstring& dir) {
	if (dir.empty())
		return false;
	const DWORD a = GetFileAttributesW(dir.c_str());
	if (a != INVALID_FILE_ATTRIBUTES)
		return (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
	const int r = SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
	return r == ERROR_SUCCESS || r == ERROR_ALREADY_EXISTS || r == ERROR_FILE_EXISTS;
}

static void collectPresets(const std::wstring& dir, std::vector<std::wstring>& out, int depth) {
	if (depth > 8)
		return;
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do {
		const std::wstring name = fd.cFileName;
		if (name == L"." || name == L"..")
			continue;
		const std::wstring full = dir + L"\\" + name;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
				collectPresets(full, out, depth + 1);
		}
		else if (hasExtension(name, L".vstpreset")) {
			out.push_back(full);
		}
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

std::vector<std::wstring> listPresetFiles(const std::wstring& dir) {
	std::vector<std::wstring> out;
	if (!dir.empty())
		collectPresets(dir, out, 0);
	std::sort(out.begin(), out.end(), [](const std::wstring& a, const std::wstring& b) {
		const std::wstring na = lowerW(stem(a)), nb = lowerW(stem(b));
		return na != nb ? na < nb : lowerW(a) < lowerW(b);
	});
	return out;
}

std::string presetDisplayName(const std::wstring& path) {
	return toUtf8(stem(path));
}

bool readFile(const std::wstring& path, std::vector<char>& out) {
	FILE* f = _wfopen(path.c_str(), L"rb");
	if (!f)
		return false;
	bool ok = false;
	if (_fseeki64(f, 0, SEEK_END) == 0) {
		const long long n = _ftelli64(f);
		if (n >= 0 && n <= 64LL * 1024 * 1024 && _fseeki64(f, 0, SEEK_SET) == 0) {
			out.resize(size_t(n));
			ok = n == 0 || std::fread(out.data(), 1, size_t(n), f) == size_t(n);
		}
	}
	std::fclose(f);
	return ok;
}

bool writeFile(const std::wstring& path, const std::vector<char>& data) {
	const std::wstring dir = parentDir(path);
	if (!dir.empty())
		ensureDir(dir);
	FILE* f = _wfopen(path.c_str(), L"wb");
	if (!f)
		return false;
	const bool ok = data.empty() || std::fwrite(data.data(), 1, data.size(), f) == data.size();
	return (std::fclose(f) == 0) && ok;
}

} // namespace presetTools
