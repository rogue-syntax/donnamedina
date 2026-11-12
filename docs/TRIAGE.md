# Triage: "look into, possibly fix later"

## MUST FIX
- ~~**B-18** note-off lost during the ~21 ms retrigger fade → hanging/ghost note~~ **fixed 2026-11-12** with B-05; regression test `tests/note_off`. Confirm by ear in FL Studio.


Items are logged as *look into / possibly fix*; struck-through ones are fixed (each with a fix log and test in `BUGS.md`). Grouping is by the symptom you would notice, not by priority. Last updated 2026-11-12 (after the fix session).

## Notes for later (from you)
- ~~**Review the try/catch blocks everywhere**~~ **done 2026-11-12** (15 GUI-handler blocks removed and replaced by real checks, one boundary kept in `drawOpenGL`, registry-read catch kept; see BUGS.md "Fix log: try/catch review"). (GUI handlers, registry read, `NVGView` `on*` handlers). They are probably a band-aid for bad C++ cleanup or timing problems; find what they were hiding and remove them where the real cause can be fixed. Note: until 2026-11-12 Release built with exceptions off, so those catches did not work reliably anyway (B-50). Related: B-33 (null derefs inside try blocks are not exceptions).

## Likely related to symptoms in your old notes
| ID | What | Evidence level |
|---|---|---|
| ~~B-03~~ | ~~Chunked processing shifts host output pointers, overruns buffer~~ **fixed 2026-11-12**, test `tests/note_off` | reproduced and verified in the real code |
| ~~B-05, B-18~~ | ~~Same-sample off/on reorder; note-off lost in retrigger fade → hanging note~~ **fixed 2026-11-12**, test `tests/note_off` | reproduced and verified in the real code |
| ~~B-06~~ | ~~Velocity is a coarse step curve (velocities 64-127 all equal)~~ **fixed 2026-11-12** (linear), test `tests/note_off` | measured in the real code |
| ~~B-17~~ | ~~Voices never leave the active list~~ **fixed 2026-11-12** (also enforces the 32-voice limit), test `tests/note_off` | reproduced and verified in the real code |
| ~~B-21~~ | ~~TRI shape discontinuity / overshoot with width~~ **fixed 2026-11-12** (mirror above 0.5 + polyBLEP), test `tests/note_off` | measured on the real oscillator; link to the "buzz" still to confirm by ear |
| ~~B-20~~ | ~~`recalcWidth` writes wrong variable~~ **fixed 2026-11-12**, test `tests/note_off` | reproduced |
| ~~B-28~~, ~~B-29~~, ~~B-54~~ | ~~NanoVG context deleted without context current; dangling `nvgViewController`; font stash leaked per editor close (0.37 MB)~~ **fixed 2026-11-12**, test `tests/editor_cycle` | measured with the real editor (200 open/close cycles) |

## Preset / state / parameter plumbing
~~B-01 pending preset never applied · B-02 controller drops host changes with editor closed~~ (fixed 2026-11-12) · B-07 missing return + magic 173 · B-08 state load robustness · B-09 instance-id compare · ~~B-11 shared static presets~~ (fixed 2026-11-12) · B-12 hardening · ~~B-23 `ctrlValues` overflow~~ (fixed 2026-11-12; was corrupting gain/filter ramps) · ~~B-55 pan smoothing dead -> pop on retrigger~~ (fixed 2026-11-12) · ~~B-27 message IDs not C strings~~ (fixed 2026-11-12)

## Crash / robustness hazards (latent)
~~B-04 null checks~~ (fixed 2026-11-12) · ~~B-10 real-time safety~~ (fixed 2026-11-12: 0 heap allocations in `process()`, messages queued to a main-thread timer) · ~~B-52 params 55-99 out of bounds / uninitialised recalculators~~ (fixed 2026-11-12) · ~~B-53 colliding 2-char message IDs~~ (fixed 2026-11-12) · ~~B-13 tempo float/double compare~~ (fixed 2026-11-12) · ~~B-39 `processContext` null~~ (fixed with B-04) · ~~B-22 osc-1 note index~~ (fixed 2026-11-12) · ~~B-29 dangling `nvgViewController`~~ (fixed 2026-11-12) · ~~B-30 editor open/close ordering~~ (fixed 2026-11-12) · ~~B-31 uninitialised CView fields~~ (fixed) · ~~B-32 `stoi` throw~~ (fixed) · ~~B-33 missing-view derefs~~ (fixed) · B-38 hard-coded view names (mitigated: exception boundary in drawOpenGL)

## Registration
B-45 HKCU + once-per-instance check · B-46 weak protection (info)

## Build / project hygiene
Done 2026-11-12: B-36, B-48, B-49, B-50 (see BUILDING.md). Open: B-51 · B-24 `RVoicesMap` leak · B-25 dead/copy-paste items · B-14/B-15/B-16 misc · B-34 dead handler

## Performance / info (unmeasured)
B-26, B-37, B-35 · B-40 denormals (mostly moot since B-17: silent voices are no longer processed; a release tail can still pass through denormal range) · B-41 Q range · B-42 filter Nyquist · B-43 LFO saw chase · ~~B-47 stale mod after changing mod type~~ (fixed 2026-11-12) · B-44 float phase accumulators

## Still unexplained from your old notes
~~"second voice drops out on retrig" / "modulation drops out on retrig when env too high"~~ explained and fixed 2026-11-12 (B-56: envelopes 2/3 jumped to 0 at the retrigger restart). "INIT modulation adds up on voice stealing": no accumulating code found in the note-on or steal path; likeliest past causes are fixed (B-23 corrupted the MOD_GAIN ramp, B-17 re-stole the fading voice, B-47 left stale mod on the carrier). Recheck by ear.

## Not yet looked at
remaining small draw code, `RSOGProc/View` (probably dead).

Out of scope (per you): mono/portamento mode and `cutNote == false` are off and not needed.
