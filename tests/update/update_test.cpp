// Update check tests (store spec licensing-v1.md 1.8 and 2.5): the computer ID, the request body,
// every kind of reply, version order, the badge rules, the registry values (through a scratch key)
// and the worker's start conditions.
// With UPDATE_LIVE=1 it also talks to the network: one real check against the store (with the spec's
// test computer ID "rogue-syntax:test" and no licence) and the 5 s deadline against an address that
// never answers. Build and run: tests\update\build_and_run.bat
#include "../../source/updateCheck.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#define NOMINMAX
#include <windows.h>

using namespace updatecheck;

static int failures = 0, checks = 0;
static void check(bool ok, const char* what) {
	checks++;
	if (!ok) {
		failures++;
		std::printf("FAIL  %s\n", what);
	}
}

static bool parse(int status, const char* body, Reply* out = nullptr) {
	Reply r;
	const bool ok = parseReply(status, body, r);
	if (out)
		*out = r;
	return ok;
}

int main() {
	// computer ID: SHA-256 of "rogue-syntax:" + the GUID, lowercase hex (the spec's curl test uses "test")
	check(machineIdFrom("test") == "4bbda661cdb78420e6946c838ae10d681b5ec83fd0a089cfe0d6b52456326294",
	      "machine ID of 'test' matches printf 'rogue-syntax:test' | sha256sum");
	check(machineIdFrom("A1b2") != machineIdFrom("a1b2"), "machine ID keeps the GUID's case");
	{
		const std::string m = machineId();
		bool hex = m.size() == 64;
		for (char c : m)
			hex = hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
		check(hex, "this computer's ID is 64 lowercase hex characters");
	}

	// request body
	check(formBody("donnamedina", "1.0.1", "ab", "") == "product=donnamedina&version=1.0.1&machine=ab&licence=",
	      "body: the four fields, empty licence when unregistered");
	check(formBody("p", "1", "m", "RS1.a-b_c.d") == "product=p&version=1&machine=m&licence=RS1.a-b_c.d",
	      "body: a token needs no escaping");
	check(formBody("p", "1", "m", "a b+c&=") == "product=p&version=1&machine=m&licence=a%20b%2Bc%26%3D",
	      "body: other characters are percent-encoded");

	// replies
	const char* spec = "{\"msg\":\"success\", \"route\":\"route_pluginCheck\", \"action\":\"nil\", \"displayMsg\":\"\", "
	                   "\"latest\":\"1.0.3\", \"update\":true, \"url\":\"https://rogue-syntax.com/\"}";
	Reply r;
	check(parse(200, spec, &r) && r.update && r.latest == "1.0.3" && r.url == "https://rogue-syntax.com/",
	      "the spec's example reply");
	check(parse(200, "{\"msg\":\"success\", \"route\":\"route_pluginCheck\", \"action\":\"nil\", \"displayMsg\":\"\", "
	                 "\"latest\":\"\", \"update\":false, \"url\":\"https://rogue-syntax.com/\"}",
	            &r) &&
	          !r.update,
	      "the store's reply today (no version published): update false");
	check(!parse(429, spec), "HTTP 429 ignored");
	check(!parse(500, spec), "HTTP 5xx ignored");
	check(!parse(200, "{\"msg\":\"error\",\"update\":true,\"latest\":\"1.0.3\",\"url\":\"https://x.com/\"}"), "msg other than success ignored");
	check(!parse(200, "not json"), "bad JSON ignored");
	check(!parse(200, "{\"msg\":\"success\",\"update\":true,"), "truncated JSON ignored");
	check(!parse(200, ""), "empty body ignored");
	check(!parse(200, "{\"msg\":\"success\",\"latest\":\"1.0.3\",\"url\":\"https://x.com/\"}"), "missing update ignored");
	check(!parse(200, "{\"msg\":\"success\",\"update\":\"true\",\"latest\":\"1.0.3\",\"url\":\"https://x.com/\"}"),
	      "update as a string ignored");
	check(!parse(200, "{\"msg\":\"success\",\"update\":true,\"latest\":\"1.0.3\",\"url\":\"http://x.com/\"}"),
	      "plain http URL refused");
	check(!parse(200, "{\"msg\":\"success\",\"update\":true,\"latest\":\"1.0.3\",\"url\":\"javascript:alert(1)\"}"),
	      "non-web URL refused");
	check(!parse(200, "{\"msg\":\"success\",\"update\":true,\"latest\":\"1.0.3\",\"url\":\"https://x.com/a b\"}"),
	      "URL with a space refused");
	check(!parse(200, "{\"msg\":\"success\",\"update\":true,\"latest\":\"1.0.3-beta\",\"url\":\"https://x.com/\"}"),
	      "bad version refused");
	check(!parse(200, "{\"msg\":\"success\",\"update\":true,\"latest\":\"1.2.3.4.5\",\"url\":\"https://x.com/\"}"),
	      "version with 5 parts refused");
	check(parse(200, "{\"msg\":\"success\",\"update\":true,\"latest\":\"2.0\",\"url\":\"https:\\/\\/x.com\\/dl?a=1\","
	                 "\"extra\":{\"n\":[1,2,{\"k\":null}],\"s\":\"\\u00e9\\\"\"},\"num\":-1.5e3,\"nul\":null}",
	            &r) &&
	          r.url == "https://x.com/dl?a=1" && r.latest == "2.0",
	      "unknown fields (nested, escaped) ignored; escaped URL decoded");
	check(parse(200, " \r\n{ \"update\" : false , \"msg\" : \"success\" } \n"), "whitespace and any field order");

	// versions
	check(validVersion("1") && validVersion("1.0.2") && validVersion("10.20.30.40"), "valid versions");
	check(!validVersion("") && !validVersion("1.") && !validVersion(".1") && !validVersion("1..2") &&
	          !validVersion("v1") && !validVersion("1.2.3.4.5"),
	      "invalid versions");
	check(compareVersions("1.0.10", "1.0.9") > 0 && compareVersions("1.0", "1.0.0") == 0 &&
	          compareVersions("1.0.1", "1.1") < 0 && compareVersions("2", "1.9.9.9") > 0,
	      "versions compare by number, missing parts are 0");

	// badge rules
	State s;
	s.latest = "1.0.3";
	s.url = "https://rogue-syntax.com/";
	check(badgeVisible(s, "1.0.1"), "badge: newer version known");
	check(!badgeVisible(s, "1.0.3") && !badgeVisible(s, "1.1"), "badge: never once this version is the same or newer");
	s.skip = "1.0.3";
	check(!badgeVisible(s, "1.0.1") && updateNewer(s, "1.0.1"), "badge hidden for SkipVersion; the About line stays");
	s.latest = "1.0.4";
	check(badgeVisible(s, "1.0.1"), "badge: a newer latest than SkipVersion shows again");
	s.enabled = false;
	check(!badgeVisible(s, "1.0.1") && !updateNewer(s, "1.0.1"), "badge: nothing while checks are off");
	s.enabled = true;
	s.url.clear();
	check(!badgeVisible(s, "1.0.1"), "badge: no URL, no badge");

	// 24 hours
	State d;
	check(due(d, 1000), "due: never checked");
	d.lastCheck = 1000;
	check(!due(d, 1000 + 86399) && due(d, 1000 + 86400), "due: after 24 hours");
	check(due(d, 999), "due: clock went back");

	// registry values (scratch key)
	{
		const wchar_t* sub = L"Software\\Rogue-Syntax\\Registrations\\UpdateTestScratch";
		RegDeleteKeyW(HKEY_CURRENT_USER, sub);
		State a = loadState(sub);
		check(a.enabled && a.latest.empty() && a.lastCheck == 0, "fresh key: checks on, nothing stored");
		setEnabled(false, sub);
		check(!loadState(sub).enabled, "UpdateCheck = 0 turns checks off");
		setEnabled(true, sub);
		check(loadState(sub).enabled, "and on again");
		Reply rep;
		rep.update = true;
		rep.latest = "1.0.3";
		rep.url = "https://rogue-syntax.com/";
		storeReply(rep, sub);
		a = loadState(sub);
		check(a.latest == "1.0.3" && a.url == "https://rogue-syntax.com/", "answer kept (LatestVersion, UpdateURL)");
		hideVersion("1.0.3", sub);
		check(loadState(sub).skip == "1.0.3", "Hide stores SkipVersion");
		rep.update = false;
		storeReply(rep, sub);
		a = loadState(sub);
		check(a.latest.empty() && !badgeVisible(a, "1.0.1"), "update false clears the answer");
		HKEY k;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, sub, 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
			const wchar_t* v = L"1763600000";
			RegSetValueExW(k, L"LastCheck", 0, REG_SZ, reinterpret_cast<const BYTE*>(v), DWORD((wcslen(v) + 1) * 2));
			const wchar_t* bad = L"12x";
			RegSetValueExW(k, L"LatestVersion", 0, REG_SZ, reinterpret_cast<const BYTE*>(bad), DWORD((wcslen(bad) + 1) * 2));
			RegCloseKey(k);
		}
		a = loadState(sub);
		check(a.lastCheck == 1763600000ull, "LastCheck read as Unix seconds");
		check(!badgeVisible(a, "1.0.1"), "a garbled stored version shows no badge");
		RegDeleteKeyW(HKEY_CURRENT_USER, sub);
	}

	// the worker never starts without a holder, or while suppressed
	{
		const unsigned g = generation();
		startIfDue("donnamedina", "1.0.1");  // no holder: nothing
		acquire();
		suppressChecks(true);
		startIfDue("donnamedina", "1.0.1");  // suppressed: nothing
		release();
		suppressChecks(false);
		check(generation() == g, "no check without a holder or while suppressed");
		acquire();  // a new holder after the last release: requests work again (below)
	}

	const char* live = std::getenv("UPDATE_LIVE");
	if (live && *live == '1') {
		using clk = std::chrono::steady_clock;
		Reply lr;
		int status = 0;
		auto t0 = clk::now();
		const bool used = runCheck(formBody("donnamedina", "0.0.1", machineIdFrom("test"), ""), lr, &status);
		const double secs = std::chrono::duration<double>(clk::now() - t0).count();
		std::printf("live: HTTP %d, used %d, update %d, latest '%s', url '%s', %.2f s\n", status, int(used), int(lr.update),
		            lr.latest.c_str(), lr.url.c_str(), secs);
		check(used && status == 200, "live: the store answers a check");
		t0 = clk::now();
		const bool none = runCheck("x=1", lr, &status, L"10.255.255.1");
		const double dl = std::chrono::duration<double>(clk::now() - t0).count();
		std::printf("live: unreachable host gave up after %.2f s\n", dl);
		check(!none && dl < 6.5, "live: an address that never answers gives up within the 5 s deadline");
	}

	std::printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
