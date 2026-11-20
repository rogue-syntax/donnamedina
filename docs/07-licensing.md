# Licensing v1: how Donna Medina meets the store spec

**The spec is the store's**, not this file: [store/licensing-v1.md](store/licensing-v1.md) (master), with [store/registration-journey.md](store/registration-journey.md) and [store/PLUGIN-DEVELOPER-CHECKLIST.md](store/PLUGIN-DEVELOPER-CHECKLIST.md). These are verbatim copies of the store's plugin developer pack (2026-11-20, second update: EULA, trademark names; `licensing-v1.md` itself unchanged). Section numbers below (1.2, 2.3, ...) refer to `store/licensing-v1.md`. This file says how the plugin does each part, where the code is, and what is left.

**Status (2026-11-20):** plugin side built for the pack of 2026-11-20 (generic spec: test product `testplugin`, identity from the plugin name, update check, installer recipe with the EULA, trademark names). `installer\output\DonnaMedinaSetup.exe` 1.0.1 builds. Still to do: a real `dev` licence on this machine, and running the installer with a `license.txt`; see 9.

**In short:** the store signs one licence per customer with an Ed25519 private key; the plugin holds only the public key, so it can check a licence but never make one. Every customer gets the same plugin and the same installer. Before v1 the plugin checked a bcrypt hash of one password built into the source (`RSRegKey`); that is gone, and old `RSRegKey` values are ignored.

---

## 1. Identity (spec 1.7)

From the name "Donna Medina" (no tilde). These never change once the plugin ships.

| item | value | in the code |
|---|---|---|
| Product ID | `donnamedina` | `license::kProduct` (`source/license.h`) |
| Registry subkey | `DonnaMedina`: `HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina` | `license::kRegSubKey` |
| Licence prefix | `DM` (licence IDs `DM-000123`) | the store makes the IDs; the plugin only checks their form |
| Shop page (Buy) | `https://rogue-syntax.com/donna-medina` | `license::kShopUrl` |
| Installer ID (`AppGuid`) | `5B54E8DA-C62E-574D-B7D1-A0F7AD12F38E` (UUID v5, DNS namespace, `donnamedina.plugins.rogue-syntax.com`) | `installer/plugins/DonnaMedina.iss` |
| Version | `1.0.1` | `source/version.h` (`VERSION_STR`): the host, the About screen (`RSVERSION`), the update check and `installer\build.bat` all use it |

The store owner still has to confirm the prefix `DM` and the product (spec 1.7: "confirm the identity with the store owner"). The first pack (2026-11-20, superseded) had a different AppGuid (`BDF363DC-...`); nothing shipped with it.

**Names with the trademark sign** (checklist step 5): wherever the plugin draws its own name or the company's, it writes `Donna Medina™` and `Rogue Syntax Sound Design™` (`TM_SIGN`, UTF-8, in `source/constants.h`; the UI font DejaVu Sans Mono has the glyph):
- the logo art (main window and About): a small ™ drawn after DONNA MEDINA (`NVGView::drawLogoTrademark`; the art itself is unchanged);
- the readout on logo hover, `ABOUT DONNA MEDINA™` (the readout's scramble now works per UTF-8 character, `RandyTxt::scrambleFront`);
- the About footer, `©<year> Rogue Syntax Sound Design™, LLC.`; the CREDITS page heading and first line (`tools/gen_notices.ps1` writes ™ into `licenses/THIRD-PARTY-NOTICES.txt` and `source/creditsText.h`);
- "Thank you. Donna Medina™ is ready to play." after a good paste;
- the installer: the recipe adds ™ to the name and publisher itself (`PluginTitle`).

Not changed, on purpose: the name and vendor the plugin reports to the host (`source/factory.cpp`: "Donna Medina", "Rogue Syntax"), which hosts show in their plugin lists and may use to find the plugin in saved projects and their plugin databases; to settle with the store owner before the first release. Also unchanged: file and folder names (`Documents\VST3 Presets\Rogue Syntax\Donna Medina`), and `stringLegalCopyright` in `version.h` (no resource uses it).

## 2. Checking a licence (spec 1.2–1.6, 2.1)

`source/license.h/.cpp`, with Monocypher 4.0.2 in `external/monocypher/` (only `crypto_ed25519_check`, the SHA-512 variant the spec asks for; credited in `licenses/THIRD-PARTY-NOTICES.txt` and the About screen's CREDITS page through `tools/gen_notices.ps1`).

- **`verifyTokenWithKeys(token, keys, n, product, out)`**: every rule of 1.2 and 1.3 against a given key table and expected product (the tests pass `testplugin`). Whitespace anywhere in the token is removed first (1.3).
- **`verifyToken(token, out)`**: this plugin's product and built-in keys. `kid=1` (`7a51af62...8b78`) in every build; the TEST key `kid=0` only in debug builds (`_DEBUG`), so a release build rejects it (1.5).
- **Stricter than the spec, on purpose:** an unknown `kind` is rejected (a future `trial` must not unlock this version); so are a repeated key, an empty key, a control character, a `lid` or `issued` of the wrong form, and base64url with non-zero leftover bits.
- Nothing throws; a licence that can't be read or checked counts as none (2.1).

## 3. Licensed and not licensed (spec 2.2)

- **Processor** (`DX10Processor`): `refreshLicense()` checks the stored token in the constructor and on `CMSG_LICENSE_CHANGED_CHAR` (sent by its controller after a paste). `noteOn` plays only while `licensedNow()`: its own result or `license::sessionLicensed()`.
- **Controller** (`BaseController`): `refreshLicense()` in `initialize` and at every editor open; sets `licensed` and `licenseInfo`.
- **Every open instance unlocks at once:**
  - same process: a valid check or paste sets a process-wide flag (`license::sessionLicensed`), read by every processor's `noteOn` and by every open editor (`NVGView` checks it each frame and refreshes its controller);
  - other processes (DAWs that sandbox plugins): an unlicensed processor holds a registry watch (`license::watchAcquire/Release`), a background thread that waits for writes to the plugin's key (`RegNotifyChangeKeyValue`) and re-checks the token. It ends once licensed, or with the last unlicensed processor.
- **Not licensed:** no notes, and the "Not registered" screen.

## 4. What the customer sees (spec 2.3)

- **"Not registered" screen** (`NVGView::drawBadReg`, layout `editorLayout::licenseScreen`, clicks through the invisible custom view `licensePanel` / `LicenseView`, last in `vstgui.uidesc` so it is on top; mouse-enabled only while unlicensed):
  - where the key is (receipt email, `license.txt`, the installer adds it);
  - **Buy** opens the shop page; **Paste license** checks the clipboard and stores a valid key;
  - a failed paste shows "That license is not valid." (or "The clipboard is empty...", "...could not be saved.");
  - a good paste shows "Licensed to <name>" for 2.5 s, then the screen goes.
- **About screen** (`NVGView::drawMoreInfoWindow`; zones in `aboutLinks`, `AboutLinksView`):
  - `LICENSED TO <name>  (<email>)  |  DEV` (or `NFR`); the email goes first, then the name is cut on whole UTF-8 characters, if the line would run into the build stamp. `NOT REGISTERED` otherwise;
  - **PASTE LICENSE** replaces the key; its result shows on the licence line for a few seconds;
  - **CHECK FOR UPDATES** box (on by default);
  - while a newer version is known: `UPDATE AVAILABLE: <latest>` (opens the store's URL) and `HIDE UNTIL THE NEXT VERSION`. After Hide the line stays (without the Hide option) and the badge goes.
- **"UPDATE AVAILABLE" badge** right of the PRESET label (`NVGView::drawUpdateBadge`, `editorLayout::kUpdateBadge`, custom view `updateBadge` / `UpdateBadgeView`, mouse-enabled only while shown); a click opens the URL.

## 5. Update check (spec 1.8)

`source/updateCheck.h/.cpp`:

- **When:** the controller's `initialize` (the plugin opens) calls `startIfDue`: only if checks are on (`UpdateCheck` not `0`), 24 hours have passed since `LastCheck` (or the clock went back), and the computer ID can be read. `LastCheck` is written when a check starts, so a failed check waits for the next day too. One check at a time per process.
- **Request:** a background thread, WinHTTP, HTTPS with the normal certificate checks, `POST https://rogue-syntax.com/v1/route_pluginCheck/` with `product`, `version` (`VERSION_STR`), `machine` and `licence` (the stored token as stored, and only if it is valid; empty otherwise). Nothing else is sent (user agent `RogueSyntaxPlugin/1`).
- **Computer ID:** SHA-256 (Windows CNG) of `rogue-syntax:` + `HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid` (64-bit view), lowercase hex. It is only sent to the store. It never decides whether the plugin is licensed.
- **5 s deadline** for the whole exchange: a timer closes the request, which ends any blocking WinHTTP call. The controller's `terminate` (last holder) also cancels and joins the thread. Never on the audio thread.
- **Reply:** used only for HTTP 200, `msg` `success` and a boolean `update`. With `update` true it also needs a valid `latest` (digits and dots, up to 4 parts) and an `https://` `url`, or the whole reply is ignored. Unknown fields are ignored. `update` true stores `LatestVersion` and `UpdateURL`; `update` false clears them. The URL always comes from the reply.
- **Badge rule:** checks on, a stored `LatestVersion` newer than this version and not equal to `SkipVersion`.
- **Registry values** (all `REG_SZ`, in the plugin's key): `LastCheck`, `LatestVersion`, `UpdateURL`, `SkipVersion`, `UpdateCheck`.
- **The store today** (checked 2026-11-20): it answers `{"msg":"success", ... "latest":"", "update":false, "url":"https://rogue-syntax.com/"}`, because no version of `donnamedina` is published there yet.

## 6. Installer (spec 1.4, 2.4)

The store's recipe, copied into `installer/` (`rogue-syntax.iss` and `eula.txt`, both UTF-8 with a BOM, keep it; `README.md`, `.gitignore`), plus our `installer/plugins/DonnaMedina.iss` (the pack's `plugins/example.iss` filled in) and `installer/build.bat`:

- `installer\build.bat` copies `x64\Release\RSSynth001donnaMedina.vst3` to `installer\in\DonnaMedinaVST64.vst3`, reads the version from `source\version.h` and runs `ISCC plugins\DonnaMedina.iss /DPluginVersion=<version>`. The result is `installer\output\DonnaMedinaSetup.exe`. `in\` and `output\` are not committed.
- The installer shows the store's licence agreement (`eula.txt`, must be accepted; don't edit it, the store owner updates it), copies the plugin into `C:\Program Files\Common Files\VST3\DonnaMedinaVST64.vst3`, and the licence from a `license.txt` next to it into the registry as the logged-in user (anything before `RS1.` on the line, such as a BOM, is skipped). Without a `license.txt` it offers to paste the key.
- The EULA's update check paragraph (5) matches section 5 above: product, version, the licence key and a one-way code from the computer's ID; the check can be turned off.
- Needs **Inno Setup 6.3+** (6.7.3 installed for the user on the build machine, 2026-11-20; `build.bat` finds it in `%LOCALAPPDATA%\Programs`). Built 2026-11-20: `DonnaMedinaSetup.exe` 1.0.1, 3.6 MB, product "Donna Medina™", publisher "Rogue Syntax Sound Design™ LLC"; compiles without warnings. Not run yet (it installs into Program Files).
- **Hand over:** the store owner gets `DonnaMedinaSetup.exe` and its version number (checklist step 8), and uploads it in the dashboard (Products → "upload new"). The store no longer serves the bare `.vst3`.

## 7. Tests (spec 2.5)

- **`tests\license\build_and_run.bat`** (release settings, 54 checks):
  - T1–T7 and the reject list of 1.6, for `testplugin`;
  - payloads signed in the test with the TEST seed (unknown key, `v=2`, unknown kind, `nfr`, product case, another product, empty name, repeated key, control character, missing LF, missing field, bad `lid` or date, size limit, `RS2.`);
  - the release table rejects `kid=0` (a TEST-key token for `donnamedina` that the TEST table accepts);
  - Paste license results that never reach the registry; store and load through a scratch subkey.
  - `tests/license/vectors.h` comes from `go run tools/license/testvectors.go`, which reproduces the spec's T1–T6 byte for byte.
- **`tests\update\build_and_run.bat`** (44 checks): computer ID (matches the spec's `printf 'rogue-syntax:test' | sha256sum`), request body, every reply case (spec example, 429, 5xx, other `msg`, bad or truncated JSON, wrong types, http or odd URLs, bad versions, nested unknown fields), version order, badge and Hide rules, the 24 hours, the registry values through a scratch key, and the worker's start conditions. With `UPDATE_LIVE=1` (46 checks): one real check against the store with the test computer ID, and the 5 s deadline against an address that never answers.
- **`note_off`, `editor_cycle`:** both turn off update checks (`updatecheck::suppressChecks`). The editor shots have `EDITOR_SHOT_LICENSE=1|2|3` (the screen; a failed paste; the moment after a good one) and `EDITOR_SHOT_UPDATE=1` (a pretend 1.0.3: badge and About line).

## 8. Checklist (store/PLUGIN-DEVELOPER-CHECKLIST.md)

| step | state |
|---|---|
| Identity | worked out (1 above); prefix and product to confirm with the store owner |
| 1 Licence check with the production key | done |
| 2 Tests T1–T7 and reject cases | done (54/54) |
| 3 Not registered: no notes, Buy and Paste license | done |
| 4 Registered: About says Licensed to <name> | done |
| 5 Names: `Donna Medina™`, `Rogue Syntax Sound Design™` | done in the plugin's own text and the installer (1 above); the name reported to the host is unchanged, to settle |
| 6 Update check: daily, background, badge, About line, off switch | done; the store has no version to announce yet |
| 7 Dev licence pasted and unlocking | **to do**: needs a `dev` licence for `donnamedina` from the store owner |
| 8 Installer built and tested with a `license.txt` | built (with the EULA); **to do**: run it with a `license.txt` |
| 9 Hand over the `.exe` and version | **to do** |

## 9. Not done / limits

- **No real licence tested yet:** nothing signed with the production key has been pasted (needs the `dev` licence). The plugin's check is covered by the TEST-key vectors.
- **Debug build** not compiled since these changes.
- The registry watch runs where an unlicensed processor is, and sets the flag for that process (its editors see it too). A host that ran a controller in a different process from its processor would update that editor only on its next open.
- If a licence is removed from the registry, instances already licensed stay licensed until the plugin is reloaded.
- Revocation is not checked (spec 3.4: v1 records only).
