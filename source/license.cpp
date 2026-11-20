// Licensing v1, see license.h and docs/07-licensing.md.
#include "license.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "../external/monocypher/monocypher-ed25519.h"

namespace license {

namespace {

// Production key (kid 1) from the store's key setup; the TEST key (kid 0) is public
// (seed 00 01 .. 1f, tools/license/testvectors.go) and only in debug builds.
const PublicKey kKeys[] = {
	{1, {0x7a, 0x51, 0xaf, 0x62, 0x39, 0xb6, 0xed, 0x75, 0xc6, 0x95, 0xf1, 0x00, 0xcd, 0xe2, 0x94, 0xf9,
	     0x74, 0x5a, 0x06, 0xba, 0xd9, 0x32, 0xed, 0x87, 0x02, 0x36, 0xdb, 0xf4, 0xe6, 0x57, 0x8b, 0x78}},
#ifdef _DEBUG
	{0, {0x03, 0xa1, 0x07, 0xbf, 0xf3, 0xce, 0x10, 0xbe, 0x1d, 0x70, 0xdd, 0x18, 0xe7, 0x4b, 0xc0, 0x99,
	     0x67, 0xe4, 0xd6, 0x30, 0x9b, 0xa5, 0x0d, 0x5f, 0x1d, 0xdc, 0x86, 0x64, 0x12, 0x55, 0x31, 0xb8}},
#endif
};

constexpr const char* kPrefix = "RS1.";
constexpr size_t kMaxToken = 4096;  // a v1 token is at most ~1500 characters
constexpr size_t kMaxPayload = 1024;
const wchar_t* const kRegValue = L"License";

std::atomic<bool> gSessionLicensed {false};

int b64val(char c) {
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '-') return 62;
	if (c == '_') return 63;
	return -1;
}

// base64url without padding (RFC 4648 section 5). Rejects '=', other characters, impossible
// lengths and non-zero leftover bits, so every byte string has exactly one accepted encoding.
bool b64urlDecode(const std::string& s, std::vector<uint8_t>& out) {
	out.clear();
	if (s.size() % 4 == 1)
		return false;
	uint32_t acc = 0;
	int bits = 0;
	for (char c : s) {
		const int v = b64val(c);
		if (v < 0)
			return false;
		acc = (acc << 6) | uint32_t(v);
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			out.push_back(uint8_t((acc >> bits) & 0xff));
		}
	}
	return (acc & ((1u << bits) - 1)) == 0;
}

// key=value lines, each ending in '\n', split on the first '='. Unknown keys are kept and ignored
// by the caller; a repeated key, an empty key or a control character makes the payload invalid.
struct Field {
	std::string key, value;
};
bool parsePayload(const std::vector<uint8_t>& p, std::vector<Field>& fields) {
	fields.clear();
	if (p.empty() || p.back() != '\n')
		return false;
	size_t start = 0;
	while (start < p.size()) {
		size_t end = start;
		while (p[end] != '\n')
			end++;
		const std::string line(reinterpret_cast<const char*>(p.data()) + start, end - start);
		start = end + 1;
		const size_t eq = line.find('=');
		if (eq == std::string::npos || eq == 0)
			return false;
		for (unsigned char c : line)
			if (c < 0x20 || c == 0x7f)
				return false;
		Field f{line.substr(0, eq), line.substr(eq + 1)};
		for (const Field& g : fields)
			if (g.key == f.key)
				return false;
		fields.push_back(std::move(f));
	}
	return true;
}

const std::string* find(const std::vector<Field>& fields, const char* key) {
	for (const Field& f : fields)
		if (f.key == key)
			return &f.value;
	return nullptr;
}

bool parseKid(const std::string& s, int& kid) {
	if (s.empty() || s.size() > 6)
		return false;
	int v = 0;
	for (char c : s) {
		if (c < '0' || c > '9')
			return false;
		v = v * 10 + (c - '0');
	}
	kid = v;
	return true;
}

bool validLid(const std::string& s) {
	if (s.empty() || s.size() > 40)
		return false;
	for (char c : s)
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
			return false;
	return true;
}

bool validDate(const std::string& s) {
	if (s.size() != 10 || s[4] != '-' || s[7] != '-')
		return false;
	for (int i : {0, 1, 2, 3, 5, 6, 8, 9})
		if (s[i] < '0' || s[i] > '9')
			return false;
	return true;
}

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
#endif

} // namespace

bool verifyTokenWithKeys(const std::string& tokenIn, const PublicKey* keys, int keyCount, const char* expectedProduct,
                         License& out) {
	out = License();
	if (!expectedProduct || !*expectedProduct)
		return false;
	if (tokenIn.size() > kMaxToken)
		return false;
	std::string token;
	token.reserve(tokenIn.size());
	for (char c : tokenIn)
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\v' && c != '\f')
			token.push_back(c);

	const size_t pl = std::strlen(kPrefix);
	if (token.size() <= pl || token.compare(0, pl, kPrefix) != 0)
		return false;
	const size_t dot = token.find('.', pl);
	if (dot == std::string::npos || token.find('.', dot + 1) != std::string::npos)
		return false;

	std::vector<uint8_t> payload, sig;
	if (!b64urlDecode(token.substr(pl, dot - pl), payload) || !b64urlDecode(token.substr(dot + 1), sig))
		return false;
	if (payload.empty() || payload.size() > kMaxPayload || sig.size() != 64)
		return false;

	// The kid picks the key, so it is read before the signature is checked; everything else is
	// only trusted after.
	std::vector<Field> fields;
	if (!parsePayload(payload, fields))
		return false;
	const std::string* kidS = find(fields, "kid");
	int kid = -1;
	if (!kidS || !parseKid(*kidS, kid))
		return false;
	const PublicKey* key = nullptr;
	for (int i = 0; i < keyCount; i++)
		if (keys[i].kid == kid)
			key = &keys[i];
	if (!key)
		return false;
	if (crypto_ed25519_check(sig.data(), key->key, payload.data(), payload.size()) != 0)
		return false;

	const std::string* v = find(fields, "v");
	const std::string* product = find(fields, "product");
	const std::string* lid = find(fields, "lid");
	const std::string* kind = find(fields, "kind");
	const std::string* name = find(fields, "name");
	const std::string* email = find(fields, "email");
	const std::string* issued = find(fields, "issued");
	if (!v || !product || !lid || !kind || !name || !email || !issued)
		return false;
	if (*v != "1" || *product != expectedProduct || !validLid(*lid) || !validDate(*issued))
		return false;
	if (*kind != "full" && *kind != "dev" && *kind != "nfr")
		return false;
	if (name->empty() || name->size() > 80 || email->size() > 120)
		return false;

	out.kid = kid;
	out.lid = *lid;
	out.kind = *kind;
	out.name = *name;
	out.email = *email;
	out.issued = *issued;
	return true;
}

bool verifyToken(const std::string& token, License& out) {
	return verifyTokenWithKeys(token, kKeys, int(sizeof(kKeys) / sizeof(kKeys[0])), kProduct, out);
}

bool sessionLicensed() { return gSessionLicensed.load(std::memory_order_relaxed); }

bool checkStored(License& out) {
	const bool ok = verifyToken(loadStoredToken(), out);
	if (ok)
		gSessionLicensed.store(true);
	return ok;
}

PasteResult acceptToken(const std::string& token, License& out) {
	out = License();
	bool blank = true;
	for (char c : token)
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\v' && c != '\f')
			blank = false;
	if (blank)
		return PasteResult::kEmpty;
	if (!verifyToken(token, out))
		return PasteResult::kInvalid;
	if (!storeToken(token)) {
		out = License();
		return PasteResult::kNotSaved;
	}
	gSessionLicensed.store(true);
	return PasteResult::kOk;
}

#ifdef _WIN32
std::string loadStoredToken(const wchar_t* subKey) {
	wchar_t buf[kMaxToken + 1];
	DWORD cb = sizeof(buf);
	if (RegGetValueW(HKEY_CURRENT_USER, subKey ? subKey : kRegSubKey, kRegValue, RRF_RT_REG_SZ, nullptr, buf, &cb) != ERROR_SUCCESS)
		return std::string();
	return toUtf8(buf, wcsnlen(buf, kMaxToken));
}

bool storeToken(const std::string& token, const wchar_t* subKey) {
	HKEY k = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey ? subKey : kRegSubKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
		return false;
	const std::wstring w = toWide(token);
	const LONG rc = RegSetValueExW(k, kRegValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(w.c_str()),
	                               DWORD((w.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(k);
	return rc == ERROR_SUCCESS;
}

std::string clipboardText() {
	std::string s;
	if (!OpenClipboard(nullptr))
		return s;
	if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
		if (const wchar_t* w = static_cast<const wchar_t*>(GlobalLock(h))) {
			s = toUtf8(w, wcsnlen(w, kMaxToken + 1));
			GlobalUnlock(h);
		}
	}
	CloseClipboard();
	return s;
}

namespace {
// A holder that was never released (a leaked processor) leaves the thread running at exit; a joinable
// std::thread destroyed there would call std::terminate, so the static detaches it instead.
struct ExitSafeThread {
	std::thread t;
	~ExitSafeThread() {
		if (t.joinable())
			t.detach();
	}
};
std::mutex gWatchMx;
int gWatchCount = 0;
ExitSafeThread gWatch;
HANDLE gWatchStop = nullptr;

// Waits for writes to the plugin's key (a paste or the installer in another process) and re-checks.
void watchLoop(HANDLE stop) {
	HKEY k = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegSubKey, 0, nullptr, 0, KEY_NOTIFY | KEY_QUERY_VALUE, nullptr, &k,
	                    nullptr) != ERROR_SUCCESS)
		return;
	HANDLE changed = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	while (changed && !sessionLicensed()) {
		if (RegNotifyChangeKeyValue(k, FALSE, REG_NOTIFY_CHANGE_LAST_SET, changed, TRUE) != ERROR_SUCCESS)
			break;
		License l;
		if (checkStored(l))  // checked after arming, so a write just before it isn't missed
			break;
		HANDLE hs[2] = {stop, changed};
		if (WaitForMultipleObjects(2, hs, FALSE, INFINITE) != WAIT_OBJECT_0 + 1)
			break;  // stop, or an error
	}
	if (changed)
		CloseHandle(changed);
	RegCloseKey(k);
}
} // namespace

void watchAcquire() {
	std::lock_guard<std::mutex> g(gWatchMx);
	if (gWatchCount++ > 0 || sessionLicensed())
		return;
	gWatchStop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!gWatchStop)
		return;
	try {
		gWatch.t = std::thread(watchLoop, gWatchStop);
	} catch (...) {
		CloseHandle(gWatchStop);
		gWatchStop = nullptr;
	}
}

void watchRelease() {
	std::thread t;
	HANDLE stop = nullptr;
	{
		std::lock_guard<std::mutex> g(gWatchMx);
		if (gWatchCount == 0 || --gWatchCount > 0)
			return;
		t = std::move(gWatch.t);
		stop = gWatchStop;
		gWatchStop = nullptr;
	}
	if (stop)
		SetEvent(stop);
	if (t.joinable())
		t.join();
	if (stop)
		CloseHandle(stop);
}
#else
std::string loadStoredToken(const wchar_t*) { return std::string(); }
bool storeToken(const std::string&, const wchar_t*) { return false; }
std::string clipboardText() { return std::string(); }
void watchAcquire() {}
void watchRelease() {}
#endif

} // namespace license
