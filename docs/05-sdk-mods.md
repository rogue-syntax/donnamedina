# VST3 SDK / VSTGUI4 modifications (partial – from notes + file dates)

SDK root: `C:\vst3sdk\VST3_SDK`. A zipped copy exists at `C:\vst3sdk\VST3_SDK\RSVSTSDK.zip` (check what it contains: stock or modded). No stock baseline is available yet, so the list below is from `customviewNotes.txt`, `blank controller.txt` and file timestamps only. **Not yet diffed.**

## Known mods (from your notes)

- New controls in `vstgui4/vstgui/lib/controls/`:
  - `cbknob.*` (blank knob to draw over), `cbslider/cbSlider` (`CBSliderMode` enum added to `vstguifwd.h`), `cbuttons.*` (`cbbutton`, cloned from COnOffButton; ~60 KB, edited 2025-04-14), `cswitch.*`, `cbtextedit`, `cogknob.*`, `cogcontrol.h`, `ccontrol.h`.
- `lib/cview.h`: extra properties `customRSName`, `customRSType`; set in `uidescription/uiviewfactory` `UIViewFactory::createViewByName`.
- `CViewContainer::getViewsAll` added.
- New controls registered in `uidescription/detail/uicreatorviewattributes` and `uidescription/uiviewcreator` (with `getDisplayName()`), headers added to `lib/vstgui.h`.
- OpenGL view needs `iplatformopenglview` depth 24 / stencil 8 for NanoVG.
- Project also compiles `public.sdk/source/vst/vstguieditor.cpp` and `vst3editor.cpp` from the SDK tree.

## To do in pass 4
- Get a stock VSTGUI4/SDK of the same version (see `base/`, `doc/`, `README.md` for version) and diff `vstgui4/` and `public.sdk/source/vst/`. List every changed file with a 1-line reason.
- Review `cbuttons.cpp` (largest file, most recently edited) and the `uiviewcreator` registrations.
- Decide: keep editing the SDK in place, or move customizations into the plugin project / a patch set so SDK upgrades don't wipe them.

## Pass 4: reconstructed mod list (no stock baseline)

Method: no stock SDK exists on disk. `vstgui4/vstgui.zip` (Aug 2019) is **already modded** (contains cbknob etc.), and `RSVSTSDK.zip` is a Mar 2019 snapshot. So I (1) listed files in `vstgui4/vstgui`, `base`, `pluginterfaces`, `public.sdk/source` with mtime after 2019-05-01, (2) diffed those against the 2019 zip to see what changed later, (3) grepped for `RSEDITED`/`customRS` markers. Stock-ness of anything else is unknown. Files dated 2022-02-11 in `pluginterfaces`/`public.sdk/source` look like a Steinberg SDK update, not edits; I did not diff them.

**VSTGUI (clearly custom):**
- `lib/cview.h:56-61` `RSEDITED`: adds `customRSViewName`, `customRSType`, `customRSDisplayActive` to every `CView`. **Not initialised in the constructors** (no hit in `cview.cpp`).
- `uidescription/uiviewfactory.cpp:150-190`: fills them from the uidesc: `custom-view-name` starting with `#` -> name (without `#`); `sub-controller` attribute is parsed with `std::stoi` and reused as the **type number** (1 knob ... 14 shut-window, see 04-gui). No `#` prefix -> fields left untouched (uninitialised). No name attribute -> name "" and type 0.
- `lib/cviewcontainer.{h,cpp}`: `getViewsAll(ViewList&)` (recursive, includes containers); also `isChild` null check (2024).
- `uidescription/detail/uiviewcreatorattributes.h` + `uiviewcreator.cpp` (~740 diff lines vs 2019 zip, last 2024-10): registers `CBStepCtrlButton`, `CBMoreInfoButton`, `CBVerticalSwitch` view types.
- New/heavily modified controls: `controls/cbuttons.{h,cpp}` (largest, 2025-04), `cswitch.{h,cpp}`, `cbslider` (adds a pass-through `valueChanged`), `cogcontrol.h`, `cogknob.{h,cpp}` (NanoVG context creation commented out; now apparently unused), `cbknob.cpp`.
- `cbknob.cpp:453-463` (2020): in the click/drag-by-position branch the "wrap to min" case is commented out and a clamp to max added. Effect: dragging across the knob's dead zone no longer jumps to min; at the min side the value follows the pointer position instead. Intent matches "knob can't wrap". Not run.
- `platform/iplatformopenglview.h`: GL pixel format default depth 32/stencil 0 -> **24/8** (required by NanoVG stencil strokes).
- Small/housekeeping: `ccontrol.h` (`#ifndef pi` guard), `vstguifwd.h` (`(std::numeric_limits<>::max)()` for the Windows `max` macro), `vstguibase.h`, `vst3editor.h` include path, `vst3editor.cpp` (`editorOpen = false` initialiser), `cbitmap.cpp` (a stray comment), `ctooltipsupport.cpp` and `cvstguitimer.cpp` (`DEBUGLOG` renamed `debugLog`, probably to avoid a macro clash).

**Not VSTGUI:** `public.sdk/samples/vst/mda-vst3/source/*` also changed (the mda samples were edited in place; this project has its own copies in `source/`, so those edits may be old experiments). Not diffed.

### New findings from this pass
- **B-31** Custom `CView` fields uninitialised. For views whose uidesc `custom-view-name` lacks a leading `#` (e.g. `nvgView`) `customRSType` is garbage. `NVGView::attached` is safe only because every branch also requires a non-empty `customRSViewName` (empty for those views). Fragile: any code reading `customRSType` alone (e.g. `customRSDisplayActive`, also uninitialised) gets garbage.
- **B-32** `std::stoi(*subber)` in `uiviewfactory.cpp:170` throws `std::invalid_argument` if a `#`-named view has a non-numeric `sub-controller` (a normal VSTGUI usage, e.g. `nvgViewController`). Exceptions here would escape from the editor creation path. Not reproduced; depends on the uidesc content (not checked).

## Edits made during cleanup (2026-11-12)

| File | Change | Backup |
|---|---|---|
| `vstgui4/vstgui/lib/controls/cbuttons.h:18-20` | `#include "C:/D/SFML-2.5.1-64/include/SFML/revent/REvents.h"` / `REventable.h` -> `"../../../../external/revent/REvents.h"` / `REventable.h` (relative, marked `RSEDITED`) | scratchpad `sdk_backup/vstgui4/vstgui/lib/controls/cbuttons.h` (session scratch, temporary) |
| new `VST3_SDK/external/revent/` | single shared copy of revent (8 headers) copied from `C:\D\SFML-2.5.1-64\include\SFMLevent` | n/a (copy) |
| `vstgui4/vstgui/lib/cview.h:59-60` | B-31: the custom fields `customRSType` / `customRSDisplayActive` get initialisers (`= 0` / `= false`); marked `RSEDITED` | scratchpad `sdk_backup/vstgui4/vstgui/lib/cview.h.bak-group2` (session scratch, temporary) |
| `vstgui4/vstgui/uidescription/uiviewfactory.cpp:170` (+ `#include <cstdlib>`) | B-32: `std::stoi(sub-controller)` -> `std::strtol` (non-numeric gives -1 instead of throwing while the editor is built); marked `RSEDITED` | scratchpad `sdk_backup/vstgui4/vstgui/uidescription/uiviewfactory.cpp.bak-group2` (session scratch, temporary) |

Verified: `cbuttons.cpp` compiles through the SDK's own `build/vstgui4/vstgui/lib/vstgui.vcxproj` (Release x64). The prebuilt `build/lib/*/vstgui.lib` was **not** rebuilt yet; it still contains code compiled from the identical C:\D copy. Other projects that compile `cbuttons` now need nothing extra (the include is relative).
Seen while compiling: `CBMoreInfoButton::onMouseExited` / `onMouseEntered` (`cbuttons.cpp:2039,2051`) do not return a value on all paths (C4715) — undefined behaviour, to fix later.
