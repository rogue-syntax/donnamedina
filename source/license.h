#pragma once
// Licensing v1 (2026-11-19, docs/07-licensing.md): Ed25519-signed license tokens
//   RS1.<base64url(payload)>.<base64url(signature)>
// The plugin only holds public keys, so it can check a license but never make one. The token is
// stored in HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina, value "License". Processor and
// controller each verify it themselves (no shared flag).

#include <cstdint>
#include <string>

namespace license {

constexpr const char* kProduct = "donnamedina";

struct License {
	std::string lid, kind, name, email, issued;
	int kid = -1;
};

struct PublicKey {
	int kid;
	uint8_t key[32];
};

// Checks a token against an explicit key table (tests use the TEST key, kid 0). Whitespace
// anywhere in the token is ignored (pasted from a wrapped email). Never throws.
bool verifyTokenWithKeys(const std::string& token, const PublicKey* keys, int keyCount, License& out);

// The same against the plugin's built-in keys: kid 1 in release builds, plus the TEST key (kid 0)
// in debug builds (_DEBUG).
bool verifyToken(const std::string& token, License& out);

// The stored token, or "" if there is none. storeToken returns false if the registry write failed.
// subKey (under HKCU) defaults to the plugin's own; tests pass a scratch key.
std::string loadStoredToken(const wchar_t* subKey = nullptr);
bool storeToken(const std::string& token, const wchar_t* subKey = nullptr);

// Clipboard text as UTF-8, or "" (Windows CF_UNICODETEXT).
std::string clipboardText();

// Loads and verifies the stored token.
inline bool checkStored(License& out) { return verifyToken(loadStoredToken(), out); }

} // namespace license
