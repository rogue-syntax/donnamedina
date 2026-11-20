// Update check, see updateCheck.h and the store spec licensing-v1.md 1.8.
#include "updateCheck.h"

#include "license.h"

#include <atomic>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>  // Windows CNG (SHA-256), not the old libbcrypt
#include <winhttp.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "winhttp.lib")
#endif

namespace updatecheck {

namespace {

constexpr uint64_t kInterval = 24u * 60u * 60u;
constexpr DWORD kDeadlineMs = 5000;
constexpr size_t kMaxReply = 64 * 1024;
const wchar_t* const kHost = L"rogue-syntax.com";
const wchar_t* const kPath = L"/v1/route_pluginCheck/";

// ---- a small JSON reader: a flat object of strings, booleans and anything else (skipped) ----
struct JVal {
	enum Type { kString, kBool, kOther } type = kOther;
	std::string str;
	bool b = false;
};

void putUtf8(std::string& s, uint32_t cp) {
	if (cp < 0x80)
		s += char(cp);
	else if (cp < 0x800) {
		s += char(0xC0 | (cp >> 6));
		s += char(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		s += char(0xE0 | (cp >> 12));
		s += char(0x80 | ((cp >> 6) & 0x3F));
		s += char(0x80 | (cp & 0x3F));
	} else {
		s += char(0xF0 | (cp >> 18));
		s += char(0x80 | ((cp >> 12) & 0x3F));
		s += char(0x80 | ((cp >> 6) & 0x3F));
		s += char(0x80 | (cp & 0x3F));
	}
}

struct JReader {
	const std::string& s;
	size_t i = 0;
	explicit JReader(const std::string& str) : s(str) {}

	void ws() {
		while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
			i++;
	}
	bool hex4(uint32_t& v) {
		if (i + 4 > s.size())
			return false;
		v = 0;
		for (int k = 0; k < 4; k++) {
			const char c = s[i++];
			v <<= 4;
			if (c >= '0' && c <= '9') v |= uint32_t(c - '0');
			else if (c >= 'a' && c <= 'f') v |= uint32_t(c - 'a' + 10);
			else if (c >= 'A' && c <= 'F') v |= uint32_t(c - 'A' + 10);
			else return false;
		}
		return true;
	}
	bool str(std::string& out) {
		out.clear();
		if (i >= s.size() || s[i] != '"')
			return false;
		i++;
		while (i < s.size()) {
			const char c = s[i++];
			if (c == '"')
				return true;
			if (static_cast<unsigned char>(c) < 0x20)
				return false;
			if (c != '\\') {
				out += c;
				continue;
			}
			if (i >= s.size())
				return false;
			const char e = s[i++];
			switch (e) {
			case '"': out += '"'; break;
			case '\\': out += '\\'; break;
			case '/': out += '/'; break;
			case 'b': out += '\b'; break;
			case 'f': out += '\f'; break;
			case 'n': out += '\n'; break;
			case 'r': out += '\r'; break;
			case 't': out += '\t'; break;
			case 'u': {
				uint32_t cp;
				if (!hex4(cp))
					return false;
				if (cp >= 0xD800 && cp < 0xDC00) {  // surrogate pair
					uint32_t lo;
					if (i + 2 > s.size() || s[i] != '\\' || s[i + 1] != 'u')
						return false;
					i += 2;
					if (!hex4(lo) || lo < 0xDC00 || lo >= 0xE000)
						return false;
					cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
				} else if (cp >= 0xDC00 && cp < 0xE000)
					return false;
				putUtf8(out, cp);
				break;
			}
			default: return false;
			}
		}
		return false;
	}
	// skips any value (nested objects and arrays included)
	bool skip(int depth = 0) {
		if (depth > 32)
			return false;
		ws();
		if (i >= s.size())
			return false;
		const char c = s[i];
		if (c == '"') {
			std::string t;
			return str(t);
		}
		if (c == '{' || c == '[') {
			const char close = c == '{' ? '}' : ']';
			i++;
			ws();
			if (i < s.size() && s[i] == close) {
				i++;
				return true;
			}
			for (;;) {
				if (c == '{') {
					ws();
					std::string k;
					if (!str(k))
						return false;
					ws();
					if (i >= s.size() || s[i++] != ':')
						return false;
				}
				if (!skip(depth + 1))
					return false;
				ws();
				if (i >= s.size())
					return false;
				if (s[i] == ',') {
					i++;
					continue;
				}
				if (s[i] == close) {
					i++;
					return true;
				}
				return false;
			}
		}
		// number, true, false, null
		const size_t start = i;
		while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' && s[i] != ' ' && s[i] != '\t' &&
		       s[i] != '\r' && s[i] != '\n')
			i++;
		return i > start;
	}
	bool object(std::vector<std::pair<std::string, JVal>>& out) {
		out.clear();
		ws();
		if (i >= s.size() || s[i] != '{')
			return false;
		i++;
		ws();
		if (i < s.size() && s[i] == '}') {
			i++;
			ws();
			return i == s.size();
		}
		for (;;) {
			ws();
			std::string k;
			if (!str(k))
				return false;
			ws();
			if (i >= s.size() || s[i++] != ':')
				return false;
			ws();
			JVal v;
			if (i < s.size() && s[i] == '"') {
				v.type = JVal::kString;
				if (!str(v.str))
					return false;
			} else if (s.compare(i, 4, "true") == 0 && !isWord(i + 4)) {
				v.type = JVal::kBool;
				v.b = true;
				i += 4;
			} else if (s.compare(i, 5, "false") == 0 && !isWord(i + 5)) {
				v.type = JVal::kBool;
				i += 5;
			} else if (!skip())
				return false;
			out.emplace_back(std::move(k), std::move(v));
			ws();
			if (i >= s.size())
				return false;
			if (s[i] == ',') {
				i++;
				continue;
			}
			if (s[i] == '}') {
				i++;
				ws();
				return i == s.size();
			}
			return false;
		}
	}
	bool isWord(size_t at) const {
		if (at >= s.size())
			return false;
		const char c = s[at];
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
	}
};

const JVal* field(const std::vector<std::pair<std::string, JVal>>& o, const char* k) {
	const JVal* r = nullptr;
	for (const auto& kv : o)
		if (kv.first == k)
			r = &kv.second;  // the last one wins, as in most JSON readers
	return r;
}

bool validUrl(const std::string& u) {
	if (u.size() < 9 || u.size() > 1024 || u.compare(0, 8, "https://") != 0)
		return false;
	for (unsigned char c : u)
		if (c <= 0x20 || c >= 0x7f || c == '"')
			return false;
	return true;
}

uint64_t nowSeconds() { return uint64_t(std::time(nullptr)); }

#ifdef _WIN32
std::wstring toWide(const std::string& s) {
	if (s.empty())
		return std::wstring();
	const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
	std::wstring w(n > 0 ? n : 0, L'\0');
	if (n > 0)
		MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), &w[0], n);
	return w;
}
std::string toUtf8(const wchar_t* w, size_t len) {
	if (!w || len == 0)
		return std::string();
	const int n = WideCharToMultiByte(CP_UTF8, 0, w, int(len), nullptr, 0, nullptr, nullptr);
	std::string s(n > 0 ? n : 0, '\0');
	if (n > 0)
		WideCharToMultiByte(CP_UTF8, 0, w, int(len), &s[0], n, nullptr, nullptr);
	return s;
}
std::string regGet(HKEY root, const wchar_t* sub, const wchar_t* name, REGSAM view = 0) {
	HKEY k = nullptr;
	if (RegOpenKeyExW(root, sub, 0, KEY_QUERY_VALUE | view, &k) != ERROR_SUCCESS)
		return std::string();
	wchar_t buf[2048];
	DWORD cb = sizeof(buf);
	const LONG rc = RegGetValueW(k, nullptr, name, RRF_RT_REG_SZ, nullptr, buf, &cb);
	RegCloseKey(k);
	if (rc != ERROR_SUCCESS)
		return std::string();
	return toUtf8(buf, wcsnlen(buf, 2047));
}
void regSet(const wchar_t* sub, const wchar_t* name, const std::string& v) {
	HKEY k = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, sub, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
		return;
	const std::wstring w = toWide(v);
	RegSetValueExW(k, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(w.c_str()), DWORD((w.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(k);
}
const wchar_t* keyOr(const wchar_t* sub) { return sub ? sub : license::kRegSubKey; }

// ---- the worker ----
// A controller that was never terminated leaves the thread joinable at exit; detach rather than let
// a static std::thread call std::terminate.
struct ExitSafeThread {
	std::thread t;
	~ExitSafeThread() {
		if (t.joinable())
			t.detach();
	}
};
std::mutex gMx;
int gHolders = 0;
ExitSafeThread gWorker;
std::atomic<bool> gRunning {false};
std::atomic<bool> gStop {false};
std::atomic<HINTERNET> gReq {nullptr};
std::atomic<unsigned> gGen {0};
std::atomic<bool> gSuppressed {false};

// Closing the request handle is WinHTTP's way to cancel a blocking call on it from another thread.
void cancelRequest() {
	if (HINTERNET h = gReq.exchange(nullptr))
		WinHttpCloseHandle(h);
}
VOID CALLBACK onDeadline(PVOID, BOOLEAN) { cancelRequest(); }
#endif

} // namespace

// ---- pure helpers ----
std::string machineIdFrom(const std::string& guid) {
#ifdef _WIN32
	const std::string in = "rogue-syntax:" + guid;
	uint8_t digest[32];
	BCRYPT_ALG_HANDLE alg = nullptr;
	BCRYPT_HASH_HANDLE h = nullptr;
	bool ok = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
	ok = ok && BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) >= 0;
	ok = ok && BCryptHashData(h, reinterpret_cast<PUCHAR>(const_cast<char*>(in.data())), ULONG(in.size()), 0) >= 0;
	ok = ok && BCryptFinishHash(h, digest, sizeof(digest), 0) >= 0;
	if (h)
		BCryptDestroyHash(h);
	if (alg)
		BCryptCloseAlgorithmProvider(alg, 0);
	if (!ok)
		return std::string();
	static const char* const kHex = "0123456789abcdef";
	std::string out;
	for (uint8_t b : digest) {
		out += kHex[b >> 4];
		out += kHex[b & 15];
	}
	return out;
#else
	return std::string();
#endif
}

std::string formBody(const std::string& product, const std::string& version, const std::string& machine,
                     const std::string& licence) {
	auto enc = [](const std::string& v) {
		static const char* const kHex = "0123456789ABCDEF";
		std::string o;
		for (unsigned char c : v) {
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
			    c == '.' || c == '~')
				o += char(c);
			else {
				o += '%';
				o += kHex[c >> 4];
				o += kHex[c & 15];
			}
		}
		return o;
	};
	return "product=" + enc(product) + "&version=" + enc(version) + "&machine=" + enc(machine) + "&licence=" + enc(licence);
}

bool validVersion(const std::string& v) {
	if (v.empty() || v.size() > 40)
		return false;
	int parts = 1, digits = 0;
	for (char c : v) {
		if (c == '.') {
			if (digits == 0 || ++parts > 4)
				return false;
			digits = 0;
		} else if (c >= '0' && c <= '9') {
			if (++digits > 9)
				return false;
		} else
			return false;
	}
	return digits > 0;
}

int compareVersions(const std::string& a, const std::string& b) {
	size_t i = 0, j = 0;
	for (int part = 0; part < 4; part++) {
		unsigned long x = 0, y = 0;
		while (i < a.size() && a[i] != '.')
			x = x * 10 + unsigned(a[i++] - '0');
		while (j < b.size() && b[j] != '.')
			y = y * 10 + unsigned(b[j++] - '0');
		if (x != y)
			return x < y ? -1 : 1;
		if (i < a.size()) i++;
		if (j < b.size()) j++;
	}
	return 0;
}

bool parseReply(int httpStatus, const std::string& body, Reply& out) {
	out = Reply();
	if (httpStatus != 200 || body.size() > kMaxReply)
		return false;
	std::vector<std::pair<std::string, JVal>> o;
	JReader r(body);
	if (!r.object(o))
		return false;
	const JVal* msg = field(o, "msg");
	const JVal* upd = field(o, "update");
	if (!msg || msg->type != JVal::kString || msg->str != "success" || !upd || upd->type != JVal::kBool)
		return false;
	Reply rep;
	rep.update = upd->b;
	if (rep.update) {
		const JVal* latest = field(o, "latest");
		const JVal* url = field(o, "url");
		if (!latest || latest->type != JVal::kString || !validVersion(latest->str) || !url ||
		    url->type != JVal::kString || !validUrl(url->str))
			return false;
		rep.latest = latest->str;
		rep.url = url->str;
	}
	out = rep;
	return true;
}

bool updateNewer(const State& s, const char* ownVersion) {
	return s.enabled && validVersion(s.latest) && validUrl(s.url) && ownVersion && validVersion(ownVersion) &&
	       compareVersions(s.latest, ownVersion) > 0;
}

bool badgeVisible(const State& s, const char* ownVersion) {
	return updateNewer(s, ownVersion) && s.latest != s.skip;
}

bool due(const State& s, uint64_t now) {
	return s.lastCheck == 0 || now < s.lastCheck || now - s.lastCheck >= kInterval;
}

#ifdef _WIN32
// ---- registry ----
State loadState(const wchar_t* subKey) {
	const wchar_t* k = keyOr(subKey);
	State s;
	s.enabled = regGet(HKEY_CURRENT_USER, k, L"UpdateCheck") != "0";
	s.latest = regGet(HKEY_CURRENT_USER, k, L"LatestVersion");
	s.url = regGet(HKEY_CURRENT_USER, k, L"UpdateURL");
	s.skip = regGet(HKEY_CURRENT_USER, k, L"SkipVersion");
	const std::string lc = regGet(HKEY_CURRENT_USER, k, L"LastCheck");
	uint64_t v = 0;
	bool ok = !lc.empty() && lc.size() <= 19;
	for (char c : lc) {
		if (c < '0' || c > '9')
			ok = false;
		else
			v = v * 10 + uint64_t(c - '0');
	}
	s.lastCheck = ok ? v : 0;
	return s;
}

void setEnabled(bool on, const wchar_t* subKey) { regSet(keyOr(subKey), L"UpdateCheck", on ? "1" : "0"); }
void hideVersion(const std::string& latest, const wchar_t* subKey) { regSet(keyOr(subKey), L"SkipVersion", latest); }

void storeReply(const Reply& r, const wchar_t* subKey) {
	// "update": false clears the answer, so the badge goes away after updating
	regSet(keyOr(subKey), L"LatestVersion", r.update ? r.latest : std::string());
	regSet(keyOr(subKey), L"UpdateURL", r.update ? r.url : std::string());
}

std::string machineId() {
	const std::string guid = regGet(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid",
	                                KEY_WOW64_64KEY);
	return guid.empty() ? std::string() : machineIdFrom(guid);
}

// ---- the request ----
bool runCheck(const std::string& body, Reply& out, int* httpStatus, const wchar_t* host, const wchar_t* path) {
	out = Reply();
	if (httpStatus)
		*httpStatus = 0;
	const wchar_t* agent = L"RogueSyntaxPlugin/1";
	HINTERNET s = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!s)  // before Windows 8.1
		s = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!s)
		return false;
	WinHttpSetTimeouts(s, int(kDeadlineMs), int(kDeadlineMs), int(kDeadlineMs), int(kDeadlineMs));
	HINTERNET c = WinHttpConnect(s, host ? host : kHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
	HINTERNET r = c ? WinHttpOpenRequest(c, L"POST", path ? path : kPath, nullptr, WINHTTP_NO_REFERER,
	                                     WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
	                : nullptr;
	std::string reply;
	int status = 0;
	if (r) {
		gReq.store(r);
		// one deadline for the whole exchange: the timer closes the request, which ends any blocking call
		HANDLE timer = nullptr;
		if (!CreateTimerQueueTimer(&timer, nullptr, onDeadline, nullptr, kDeadlineMs, 0, WT_EXECUTEONLYONCE))
			timer = nullptr;
		if (gStop)
			cancelRequest();
		bool ok = WinHttpSendRequest(r, L"Content-Type: application/x-www-form-urlencoded\r\n", DWORD(-1L),
		                             const_cast<char*>(body.data()), DWORD(body.size()), DWORD(body.size()), 0) &&
		          WinHttpReceiveResponse(r, nullptr);
		if (ok) {
			DWORD code = 0, len = sizeof(code);
			if (WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
			                        &code, &len, WINHTTP_NO_HEADER_INDEX))
				status = int(code);
			char buf[4096];
			for (;;) {
				DWORD got = 0;
				if (!WinHttpReadData(r, buf, sizeof(buf), &got)) {
					ok = false;
					break;
				}
				if (got == 0)
					break;
				reply.append(buf, got);
				if (reply.size() > kMaxReply) {
					ok = false;
					break;
				}
			}
		}
		if (timer)
			DeleteTimerQueueTimer(nullptr, timer, INVALID_HANDLE_VALUE);  // waits for a running callback
		cancelRequest();  // closes the request if the deadline didn't
		if (!ok)
			status = 0;
	}
	if (c)
		WinHttpCloseHandle(c);
	WinHttpCloseHandle(s);
	if (httpStatus)
		*httpStatus = status;
	return status != 0 && parseReply(status, reply, out);
}

void acquire() {
	std::lock_guard<std::mutex> g(gMx);
	if (gHolders++ == 0)
		gStop = false;  // the last release stopped the worker; a new holder may start checks again
}

void release() {
	std::thread t;
	{
		std::lock_guard<std::mutex> g(gMx);
		if (gHolders == 0 || --gHolders > 0)
			return;
		gStop = true;
		t = std::move(gWorker.t);
	}
	cancelRequest();
	if (t.joinable())
		t.join();
}

void startIfDue(const char* product, const char* version) {
	std::lock_guard<std::mutex> g(gMx);
	if (gHolders == 0 || gRunning || gSuppressed || !product || !version)
		return;
	const State st = loadState();
	const uint64_t now = nowSeconds();
	if (!st.enabled || !due(st, now))
		return;
	const std::string machine = machineId();
	if (machine.empty())
		return;  // spec: no computer ID, no check
	if (gWorker.t.joinable())
		gWorker.t.join();  // the last one has finished (gRunning is false)
	regSet(license::kRegSubKey, L"LastCheck", std::to_string(now));
	// the stored token exactly as stored, but only when it is a valid licence (empty when unregistered)
	std::string token = license::loadStoredToken();
	license::License l;
	if (!license::verifyToken(token, l))
		token.clear();
	const std::string body = formBody(product, version, machine, token);
	gStop = false;
	gRunning = true;
	try {
		gWorker.t = std::thread([body] {
			Reply rep;
			if (!gStop && runCheck(body, rep) && !gStop) {
				storeReply(rep);
				gGen++;
			}
			gRunning = false;
		});
	} catch (...) {
		gRunning = false;
	}
}

void suppressChecks(bool on) { gSuppressed = on; }
#else
void suppressChecks(bool) {}
State loadState(const wchar_t*) { return State(); }
void setEnabled(bool, const wchar_t*) {}
void hideVersion(const std::string&, const wchar_t*) {}
void storeReply(const Reply&, const wchar_t*) {}
std::string machineId() { return std::string(); }
bool runCheck(const std::string&, Reply& out, int* httpStatus, const wchar_t*, const wchar_t*) {
	out = Reply();
	if (httpStatus)
		*httpStatus = 0;
	return false;
}
void acquire() {}
void release() {}
void startIfDue(const char*, const char*) {}
#endif

unsigned generation() {
#ifdef _WIN32
	return gGen.load();
#else
	return 0;
#endif
}

} // namespace updatecheck
