#pragma once
// Update check (store spec licensing-v1.md 1.8, docs/07-licensing.md): once every 24 hours, when the
// plugin opens, a background POST to https://rogue-syntax.com/v1/route_pluginCheck/ with the product
// ID, the plugin version, the computer ID and the stored licence token. The reply says whether a newer
// version exists; the answer is kept in the plugin's registry key (LatestVersion, UpdateURL) so the
// "Update available" badge shows between checks and offline. Never on the audio thread, 5 s deadline,
// never changes whether the plugin is licensed. Off switch: UpdateCheck = "0".

#include <cstdint>
#include <string>

namespace updatecheck {

// ---- pure helpers (tests/update) ----
struct Reply {
	bool update = false;
	std::string latest, url;
};
// lowercase hex SHA-256 of "rogue-syntax:" + guid (UTF-8), or "" if hashing failed
std::string machineIdFrom(const std::string& guid);
std::string formBody(const std::string& product, const std::string& version, const std::string& machine,
                     const std::string& licence);
// true only for HTTP 200, msg "success", a boolean "update" and (when update is true) a valid
// version and an https URL; anything else is ignored (spec 1.8 "Anything else")
bool parseReply(int httpStatus, const std::string& body, Reply& out);
bool validVersion(const std::string& v);  // digits and dots, 1..4 parts
int compareVersions(const std::string& a, const std::string& b);  // <0, 0, >0

struct State {
	bool enabled = true;            // UpdateCheck != "0"
	std::string latest, url, skip;  // LatestVersion, UpdateURL, SkipVersion
	uint64_t lastCheck = 0;         // LastCheck, Unix seconds
};
// the badge: checks on, a stored answer newer than this version and not hidden
bool badgeVisible(const State& s, const char* ownVersion);
// the About line: the same, but also while hidden (it then has no Hide option)
bool updateNewer(const State& s, const char* ownVersion);
bool due(const State& s, uint64_t now);  // 24 hours since LastCheck (or a clock that went back)

// ---- registry (the plugin's key, license::kRegSubKey; tests pass a scratch key) ----
State loadState(const wchar_t* subKey = nullptr);
void setEnabled(bool on, const wchar_t* subKey = nullptr);
void hideVersion(const std::string& latest, const wchar_t* subKey = nullptr);  // SkipVersion
void storeReply(const Reply& r, const wchar_t* subKey = nullptr);
std::string machineId();  // from HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid (64-bit view), or ""

// ---- the check ----
// One blocking request (the worker; tests). Returns true if the reply was usable (out filled).
bool runCheck(const std::string& body, Reply& out, int* httpStatus = nullptr, const wchar_t* host = nullptr,
              const wchar_t* path = nullptr);

// Controllers hold the worker between initialize and terminate; the last release cancels a request
// in flight and joins the thread (never from DllMain).
void acquire();
void release();
// Starts a background check if checks are on and one is due; never blocks.
void startIfDue(const char* product, const char* version);
// Tests that open the plugin: never start a check (no network, no LastCheck write).
void suppressChecks(bool on);
// Bumped whenever a check stored a new answer (the editor reloads its State when it changes).
unsigned generation();

} // namespace updatecheck
