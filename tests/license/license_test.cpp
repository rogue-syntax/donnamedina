// Licensing v1 tests (docs/07-licensing.md 1.6 and 2.5): the shared vectors T1-T7, the extra reject
// cases (tokens signed here with the TEST key), the release key table, and store/load through a
// scratch registry key. Build and run: tests\license\build_and_run.bat
#include "../../source/license.h"
#include "../../external/monocypher/monocypher-ed25519.h"
#include "vectors.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define NOMINMAX
#include <windows.h>

static int failures = 0, checks = 0;
static void check(bool ok, const char* what) {
	checks++;
	if (!ok) {
		failures++;
		std::printf("FAIL  %s\n", what);
	}
}

static void hexTo(const char* hex, uint8_t* out, int n) {
	for (int i = 0; i < n; i++) {
		unsigned v = 0;
		std::sscanf(hex + 2 * i, "%2x", &v);
		out[i] = uint8_t(v);
	}
}

static std::string b64url(const std::string& in) {
	static const char* a = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	std::string out;
	uint32_t acc = 0;
	int bits = 0;
	for (unsigned char c : in) {
		acc = (acc << 8) | c;
		bits += 8;
		while (bits >= 6) {
			bits -= 6;
			out.push_back(a[(acc >> bits) & 63]);
		}
	}
	if (bits > 0)
		out.push_back(a[(acc << (6 - bits)) & 63]);
	return out;
}

static uint8_t gSecret[64];
static license::PublicKey gTestKey;

// signs payload with the TEST key and builds a token
static std::string sign(const std::string& payload, const char* prefix = "RS1.") {
	uint8_t sig[64];
	crypto_ed25519_sign(sig, gSecret, reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
	return std::string(prefix) + b64url(payload) + "." + b64url(std::string(reinterpret_cast<char*>(sig), 64));
}

static std::string payload(const char* extra = "", const char* v = "1", const char* kind = "full",
                           const char* product = "testplugin", const char* name = "Jane Q. Tester") {
	return std::string("v=") + v + "\nproduct=" + product + "\nkid=0\nlid=DM-000123\nkind=" + kind + "\nname=" + name +
	       "\nemail=jane@example.com\nissued=2026-11-19\n" + extra;
}

// the spec's vectors and the payloads signed here are for the test product "testplugin" (spec 1.6)
static bool ok(const std::string& t, license::License* out = nullptr, const char* product = "testplugin") {
	license::License l;
	const bool r = license::verifyTokenWithKeys(t, &gTestKey, 1, product, l);
	if (out)
		*out = l;
	return r;
}

int main() {
	uint8_t seed[32];
	hexTo(kTestSeedHex, seed, 32);
	crypto_ed25519_key_pair(gSecret, gTestKey.key, seed);
	gTestKey.kid = 0;
	{
		uint8_t pub[32];
		hexTo(kTestPubHex, pub, 32);
		check(std::memcmp(pub, gTestKey.key, 32) == 0, "TEST seed gives the TEST public key");
	}

	// shared vectors
	license::License l;
	check(ok(T1_valid_full, &l), "T1 valid");
	check(l.name == "Jane Q. Tester" && l.email == "jane@example.com" && l.lid == "TEST-0001" && l.kind == "full" &&
	          l.issued == "2026-11-19" && l.kid == 0,
	      "T1 fields");
	check(ok(T2_valid_dev, &l) && l.kind == "dev" && l.name == "Rogue Syntax Dev", "T2 valid, kind dev");
	check(ok(T3_valid_utf8, &l) && l.name == "Zo\xc3\xab M\xc3\xbcller = DJ", "T3 valid, UTF-8 name with '='");
	check(!ok(T4_tampered), "T4 tampered payload rejected");
	check(!ok(T5_wrong_product), "T5 other product rejected");
	check(!ok(T6_unknown_kid), "T6 unknown kid rejected");
	{
		const std::string t1 = T1_valid_full;
		std::string w = "  ";
		for (size_t i = 0; i < t1.size(); i += 40)
			w += t1.substr(i, 40) + "\r\n";
		check(ok(w, &l) && l.lid == "TEST-0001", "T7 wrapped (spaces, CRLF every 40) valid");
		check(ok("\t" + t1 + " \n"), "tabs and trailing newline ignored");
	}

	// extra reject cases
	const std::string t1 = T1_valid_full;
	check(!ok(""), "empty rejected");
	check(!ok("DM1." + t1.substr(4)), "old DM1. prefix rejected");
	check(!ok(t1.substr(4)), "no prefix rejected");
	check(!ok("rs1." + t1.substr(4)), "lowercase prefix rejected");
	check(!ok(t1 + ".AAAA"), "three parts rejected");
	check(!ok(t1.substr(0, t1.find('.', 4))), "missing signature rejected");
	check(!ok(t1 + "="), "padding rejected");
	{
		std::string t = t1;
		t[10] = '+';
		check(!ok(t), "standard base64 '+' rejected");
	}
	check(!ok(t1.substr(0, t1.size() - 2)), "short signature rejected");
	check(!ok(t1 + "AA"), "long signature rejected");
	{
		std::string t = t1;
		t.back() = t.back() == 'w' ? 'x' : 'w';  // flips leftover bits / the last signature byte
		check(!ok(t), "changed signature rejected");
	}

	// signed here with the TEST key
	check(ok(sign(payload())), "locally signed baseline valid");
	check(ok(sign(payload("future=thing\n"))), "unknown key ignored");
	check(ok(sign(payload("note=a=b=c\n"))), "unknown key with '=' ignored");
	check(!ok(sign(payload("", "2"))), "v=2 rejected");
	check(!ok(sign(payload("", "1", "trial"))), "unknown kind rejected");
	check(ok(sign(payload("", "1", "nfr"))), "kind nfr valid");
	check(!ok(sign(payload("", "1", "full", "Testplugin"))), "product is case sensitive");
	check(!ok(sign(payload("", "1", "full", "testplugin", ""))), "empty name rejected");
	check(!ok(T1_valid_full, nullptr, "donnamedina"), "a plugin rejects another product's license (T1 for testplugin)");
	check(ok(sign(payload("", "1", "full", "donnamedina")), nullptr, "donnamedina"), "own product accepted");
	check(!ok(sign(payload()), nullptr, ""), "empty expected product rejects everything");
	check(!ok(sign(payload("name=Second\n"))), "repeated key rejected");
	check(!ok(sign(payload("bad\x01=x\n"))), "control character rejected");
	check(!ok(sign(payload("x=y"))), "last line without LF rejected");
	check(!ok(sign(payload("=x\n"))), "empty key rejected");
	check(!ok(sign(payload("noequals\n"))), "line without '=' rejected");
	check(ok(sign(std::string("v=1\nproduct=testplugin\nkid=0\nlid=DM-1\nkind=full\nname=A\nemail=a@b\nissued=2026-11-19\n"))),
	      "minimal payload valid (the cases below change one thing)");
	check(!ok(sign(std::string("v=1\nproduct=testplugin\nkid=0\nlid=DM-1\nkind=full\nname=A\nemail=a@b\n"))),
	      "missing issued rejected");
	check(!ok(sign(std::string("v=1\nproduct=testplugin\nkid=0\nlid=DM 1\nkind=full\nname=A\nemail=a@b\nissued=2026-11-19\n"))),
	      "bad lid rejected");
	check(!ok(sign(std::string("v=1\nproduct=testplugin\nkid=0\nlid=DM-1\nkind=full\nname=A\nemail=a@b\nissued=2026-1-19\n"))),
	      "bad date rejected");
	check(!ok(sign(payload(("pad=" + std::string(1100, 'x') + "\n").c_str()))), "payload over 1024 bytes rejected");
	check(ok(sign(payload(("pad=" + std::string(1020 - payload().size() - 5, 'x') + "\n").c_str()))),
	      "payload of 1020 bytes valid");
	check(!ok(sign(payload(), "RS2.")), "RS2. prefix rejected");

	// the plugin's built-in table: release builds hold kid 1 only
	{
		license::License r;
		const std::string own = sign(payload("", "1", "full", license::kProduct));  // this plugin, TEST key
		check(ok(own, nullptr, license::kProduct), "TEST-key token for this plugin valid against the TEST table");
		check(!license::verifyToken(own, r), "release key table rejects the TEST key (kid 0)");
		check(!license::verifyToken(sign(std::string(
		              "v=1\nproduct=donnamedina\nkid=1\nlid=DM-1\nkind=full\nname=A\nemail=a@b\nissued=2026-11-19\n")),
		          r),
		      "kid 1 signed with the wrong key rejected");
	}

	// Paste license results that never reach the registry
	{
		license::License r;
		check(license::acceptToken(" \r\n\t", r) == license::PasteResult::kEmpty, "paste: blank clipboard is 'empty'");
		check(license::acceptToken(T5_wrong_product, r) == license::PasteResult::kInvalid, "paste: invalid token refused");
		check(!license::sessionLicensed(), "nothing valid seen: not licensed for the session");
	}

	// store / load through a scratch key (not the plugin's own)
	{
		const wchar_t* sub = L"Software\\Rogue-Syntax\\Registrations\\LicenseTestScratch";
		const std::string wrapped = "  " + std::string(T3_valid_utf8) + "\r\n";
		check(license::storeToken(wrapped, sub), "store");
		const std::string back = license::loadStoredToken(sub);
		check(back == wrapped, "load returns what was stored");
		check(ok(back, &l) && l.name == "Zo\xc3\xab M\xc3\xbcller = DJ", "stored UTF-8 token still valid");
		RegDeleteKeyW(HKEY_CURRENT_USER, sub);
		check(license::loadStoredToken(sub).empty(), "missing value loads as empty");
	}

	std::printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
