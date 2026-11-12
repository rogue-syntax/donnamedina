# Triage: "look into, possibly fix later"

## MUST FIX
- ~~**B-18** note-off lost during the ~21 ms retrigger fade → hanging/ghost note~~ **fixed 2026-11-12** with B-05; regression test `tests/note_off`. Confirm by ear in FL Studio.


Discovery mode: **nothing here is scheduled for a fix.** Every item below is logged as *look into / possibly fix*. Details and evidence are in `BUGS.md` under the same ID. Grouping is by the symptom you would notice, not by priority. Last updated pass 5.

## Notes for later (from you)
- **Review the try/catch blocks everywhere** (GUI handlers, registry read, `NVGView` `on*` handlers). They are probably a band-aid for bad C++ cleanup or timing problems; find what they were hiding and remove them where the real cause can be fixed. Note: until 2026-11-12 Release built with exceptions off, so those catches did not work reliably anyway (B-50). Related: B-33 (null derefs inside try blocks are not exceptions).

## Likely related to symptoms in your old notes
| ID | What | Evidence level |
|---|---|---|
| ~~B-03~~ | ~~Chunked processing shifts host output pointers, overruns buffer~~ **fixed 2026-11-12**, test `tests/note_off` | reproduced and verified in the real code |
| B-05, B-18 | Same-sample off/on reorder (mostly harmless); note-off lost in retrigger fade → hanging note when gate < ~21 ms on a still-sounding repeated pitch | code + model, see 06 |
| ~~B-06~~ | ~~Velocity is a coarse step curve (velocities 64-127 all equal)~~ **fixed 2026-11-12** (linear), test `tests/note_off` | measured in the real code |
| ~~B-17~~ | ~~Voices never leave the active list~~ **fixed 2026-11-12** (also enforces the 32-voice limit), test `tests/note_off` | reproduced and verified in the real code |
| ~~B-21~~ | ~~TRI shape discontinuity / overshoot with width~~ **fixed 2026-11-12** (mirror above 0.5 + polyBLEP), test `tests/note_off` | measured on the real oscillator; link to the "buzz" still to confirm by ear |
| B-20 | `recalcWidth` writes wrong variable | code |
| B-28 | NanoVG GL context deleted without context current (+30 MB per open?) | code; unmeasured |

## Preset / state / parameter plumbing
~~B-01 pending preset never applied · B-02 controller drops host changes with editor closed~~ (fixed 2026-11-12) · B-07 missing return + magic 173 · B-08 state load robustness · B-09 instance-id compare · B-11 shared static presets · B-12 hardening · B-23 `ctrlValues` overflow (layout known; minor effect) · B-27 message IDs not C strings (processor side fixed; controller side open)

## Crash / robustness hazards (latent)
~~B-04 null checks~~ (fixed 2026-11-12) · ~~B-10 real-time safety~~ (fixed 2026-11-12: 0 heap allocations in `process()`, messages queued to a main-thread timer) · B-52 params 55-99 out of bounds (latent) · B-53 colliding 2-char message IDs (see B-27) · ~~B-13 tempo float/double compare~~ (fixed 2026-11-12) · ~~B-39 `processContext` null~~ (fixed with B-04) · B-22 osc-1 note index · B-29 dangling `nvgViewController` · B-30 editor open/close ordering · B-31 uninitialised CView fields · B-32 `stoi` throw · B-33 missing-view derefs · B-38 hard-coded view names

## Registration
B-45 HKCU + once-per-instance check · B-46 weak protection (info)

## Build / project hygiene
Done 2026-11-12: B-36, B-48, B-49, B-50 (see BUILDING.md). Open: B-51 · B-24 `RVoicesMap` leak · B-25 dead/copy-paste items · B-14/B-15/B-16 misc · B-34 dead handler

## Performance / info (unmeasured)
B-26, B-37, B-35 · B-40 denormals (could explain CPU growth with B-17) · B-41 Q range · B-42 filter Nyquist · B-43 LFO saw chase · B-47 stale mod after changing mod type · B-44 float phase accumulators

## Still unexplained from your old notes
"modulation drops out on retrigger when env too high" (B-19 withdrawn), "INIT modulation adds up on voice stealing", "second voice drops out on retrig" (only partly explained by B-05/B-18).

## Not yet looked at
remaining small draw code, `RSOGProc/View` (probably dead).

Out of scope (per you): mono/portamento mode and `cutNote == false` are off and not needed.
