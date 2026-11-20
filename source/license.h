#pragma once
// Licensing v1 (store spec licensing-v1.md, our notes in docs/07-licensing.md): Ed25519-signed
// license tokens
//   RS1.<base64url(payload)>.<base64url(signature)>
// The plugin only holds public keys, so it can check a license but never make one. The token is
// stored in HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina, value "License". Processor and
// controller each verify it themselves; a successful check also sets a process-wide flag, so a paste
// in one instance unlocks every instance in the DAW (spec 2.2).

#include <cstdint>
#include <string>

namespace license {

// Plugin identity (spec 1.7, from the name "Donna Medina"). Never change once shipped.
constexpr const char* kProduct = "donnamedina";
constexpr const wchar_t* kRegSubKey = L"Software\\Rogue-Syntax\\Registrations\\DonnaMedina";
constexpr const wchar_t* kShopUrl = L"https://rogue-syntax.com/donna-medina";  // the Buy button

struct License {
	std::string lid, kind, name, email, issued;
	int kid = -1;
};

struct PublicKey {
	int kid;
	uint8_t key[32];
};

// Checks a token against an explicit key table and product ID (the spec's vectors are for
// "testplugin"). Whitespace anywhere in the token is ignored (pasted from a wrapped email). Never throws.
bool verifyTokenWithKeys(const std::string& token, const PublicKey* keys, int keyCount, const char* product,
                         License& out);

// The same for this plugin's product against its built-in keys: kid 1 in release builds, plus the
// TEST key (kid 0) in debug builds (_DEBUG).
bool verifyToken(const std::string& token, License& out);

// The stored token, or "" if there is none. storeToken returns false if the registry write failed.
// subKey (under HKCU) defaults to the plugin's own; tests pass a scratch key.
std::string loadStoredToken(const wchar_t* subKey = nullptr);
bool storeToken(const std::string& token, const wchar_t* subKey = nullptr);

// Clipboard text as UTF-8, or "" (Windows CF_UNICODETEXT).
std::string clipboardText();

// Loads and verifies the stored token; on success also sets the process-wide flag below.
bool checkStored(License& out);

// Verifies a pasted token and, if valid, stores it and sets the process-wide flag.
enum class PasteResult { kOk, kEmpty, kInvalid, kNotSaved };
PasteResult acceptToken(const std::string& token, License& out);

// True once any instance in this process has seen a valid license (a stored one or a paste).
bool sessionLicensed();

// Registry watch for other processes (DAWs that run plugins in separate processes): while at least
// one holder is unlicensed, a background thread waits for changes to the plugin's registry key and
// re-checks the stored token, setting the process-wide flag when it becomes valid. Acquire / release
// in pairs (never from DllMain); the last release stops and joins the thread.
void watchAcquire();
void watchRelease();

} // namespace license
