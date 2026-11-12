# Bug ledger

Status key: **OPEN** = found in code, not reproduced · **HYPOTHESIS** = plausible cause of a known symptom · **CHECK** = claimed fixed, needs verification · **FIXED**.
All "found" items below are from **reading code only (pass 1–2)**; none have been run or reproduced. Paths relative to `source/`.

## Verification log (re-check done after the "RVoice is live" correction)

| Item | Confidence | How checked |
|---|---|---|
| B-01 pending preset never applied | High | **FIXED 2026-11-12.** Reproduced with a headless `DX10Controller`: preset 2 selected with the editor closed -> 7 of 8 probed params kept old values. 0 after. |
| B-02 params dropped while editor closed | High | **FIXED 2026-11-12.** Reproduced headless: automation of FILTER_ not stored (0.93 kept), OCT_ (vertical switch) neither stored nor sent to the processor. Stored and sent after. GUI catch-up on editor open not testable headless. |
| B-03 output pointer corruption | High | **FIXED 2026-11-12.** Reproduced in the real `DX10Processor::process()` (host pointers moved after 258 blocks, 41852 samples written past the buffer end); 0/0 after the fix. Not run in a host. |
| B-04 stale events / null processContext | Medium | **FIXED 2026-11-12.** Reproduced in the real `process()`: null `processContext` = access violation; no event list = note-on replayed every block; note-off past block end = hanging note. All pass after the fix. (A claim that `notes[]` was uninitialised was WRONG: `initialize()` sets `notes[0]=EVENTS_DONE`. Removed.) |
| B-05 reorder kills retriggered note | Medium | `cutNote` defaults to true, `noteOff` is keyed by pitch (`DXGenerator.cpp:594`), so the mechanism holds; exact audible end-state not run. |
| B-06 velocity curve (rewritten) | High · **FIXED 2026-11-12** (measured in the real `process()`: 5 of 9 test velocities distinct, 64-127 all full level, velocity 1 silent; now linear, all distinct) | Integer-division arithmetic reproduced numerically; feeds amp env at `DXGenerator.cpp:461`. My earlier "truncation" theory was NOT supported (0/127 off by one in a float test) and was dropped. |
| B-07, B-08, B-09 | High | Straight code reads. |
| B-10 RT safety | High | Counted calls; audio-thread path via `processParameterChanges`. |
| B-11 static programParams | Medium | `programParams[0]` is a real preset (my earlier "[0] is zeros" note was wrong). Array bound is fine (tested with g++, 20 rows; MSVC not tested). Host behaviour re: controller-without-processor not tested. |
| B-12 | Low | Downgraded, see below. |
| B-13 | High | **FIXED 2026-11-12.** Reproduced in the real `process()`: at 133.3 BPM, ~807 IMessages sent from the audio thread in 3.1 s (3 per block); switching LFO sync on at 120 BPM left the free rate (10.04 instead of 4.0). 0-3 messages and correct rates after the fix. |
| B-17 | High | **FIXED 2026-11-12.** Reproduced in the real `process()`: 60 released pitches -> 60 voices still rendered (5 s render 2328 ms); 40 held notes -> 40 voices despite POLYCOUNT 32. After: 0 voices / 1078 ms; 32 voices. |
| B-18 | Medium-High | Code read + Python model of my reading (not the real code); host-dependent trigger; symptom differs from "drops out". |
| B-19 | Withdrawn | `catt = 1.0` in `retriggerVoice` (line 405) refutes it. |
| B-20 | High | Header comment documents intent; no live assignment of `widthPole`. |
| B-21 | High | **FIXED 2026-11-12.** Measured on the real oscillator: width 0.7/0.9 reached -1.8/-2.6; alias -16..-32 dB at widths != 0.5. After: within +-1 at all widths, alias 15-16 dB lower. Link to the "buzz" still unproven. |
| B-22 | Medium | Range depends on step lists I did not inspect; clamp at ±20480 Hz limits damage. |
| B-23 | **High, FIXED 2026-11-12** | Measured with processor + real controller connected (like a host): after a preset change the gain ramp went to 0 and the filter to 80 Hz within ~1.3 s (silence). The earlier "minor" estimate was wrong: `setCurrentProgram` sends ids 110-143, not only the toggles. |
| B-24 | High | Single `init()` call site, no delete. |
| B-25 | High | Straight reads; `carPitchMultiplier` has no readers in live files. |
| B-26 | Low | Unmeasured. |
| B-27 | High | Read `constants.h` definitions and call sites. |

## A. Your old notes (todos.txt / issues.txt / sig proccs.txt / revents.txt)

| Symptom | Status | Notes |
|---|---|---|
| Memory +30 MB per window open | **Measured 2026-11-12** (`tests/editor_cycle`): a real leak of 0.37 MB per open/close (font stash, B-54, fixed); first open ~40 MB one-time. "+30 MB per open" itself not reproduced; recheck in FL. Original note: | NVGView/NanoVG create/destroy; `~DX10Controller` has delete commented out; verify with VS diagnostic tools: open/close editor 20x, watch heap + GPU memory. See B-14, B-18. |
| Load presets changes control views but not processor params | HYPOTHESIS → B-01, B-02, B-11 | Controller preset path vs processor program path are separate; see below. |
| Second voice drops out on retrigger | OPEN (related: B-05, B-18) | Those explain a *ghost/hanging* note on same-pitch retrigger, not a drop-out. The drop-out itself is not explained. |
| Modulation drops out on retrigger when env too high | OPEN | B-19 was withdrawn; no explanation yet. |
| Silence detection proc in retrig note | RELATED → B-17 | `detectSilence` is never called; `checkSilence` is a no-op override. Your note is ambiguous, so the match is not certain. |
| Fix velocity | HYPOTHESIS → B-06 | |
| Buzz from waveshape mod | HYPOTHESIS → B-21 (also B-20) | TRI wave jump/overshoot depends on width; audibility not confirmed. |
| INIT modulation adds up on voice stealing | OPEN | No accumulation found in `processNoteOn`/`saveTargValues` (checked); the steal path reuses voice state, see 02 doc. Needs a repro. |
| NVGView data doesn't persist across close/open | HYPOTHESIS → B-02 | Not just NVGView: controller params go stale while the editor is closed. |
| Can't get RVoice data → parallel RVoiceMap | OPEN (pass 3) | `RVoice` IS live (held by `DXGenerator`); I wrongly marked this N/A earlier. Investigate in pass 3/4. |

## B. New findings

### B-01  Pending preset is never applied  · HIGH · **FIXED 2026-11-12** with B-02 (see "Fix log: B-01 + B-02")
`DX10Controller::createView` (`mdaDX10Controller.cpp:170-185`) returns inside the `if (name == kEditor)` branch, so `checkPendingPreset()` on line 183 is only reached for *non-editor* view names (never in practice). `onNVGViewReady()` only flushes `pendingStateUpdates`, not `presetPending`.
Effect: a preset selected while the editor was closed (or before the view was "ready") is stored in `presetPending` and **never applied to the controller**, so the GUI shows old values while the processor (which got `kPresetParam` from the host) plays the new preset.
Fix: call `checkPendingPreset()` from `onNVGViewReady()` (or after the editor is attached), and make it clear the flag only after applying.

### B-02  Controller drops host param changes while editor is closed  · HIGH · **FIXED 2026-11-12** (see "Fix log: B-01 + B-02"; worse than noted: vertical-switch automation also never reached the engine while the editor was closed)
`DX10Controller::setParamNormalized` (`:771-809`): for `tag < GPARAMS` it only calls `BaseController::setParamNormalized` when `isViewReady()`. When the editor is closed, **automation, MIDI mapped CC, and preset-driven value changes are discarded** by the controller. Reopening the editor shows stale values (and hosts that read controller values, e.g. automation lanes, see stale values). Also for `tag >= 173` (still `< GPARAMS`) it calls `onStepCtrlButton` and never stores the value.
Fix: always store the value (`BaseController::setParamNormalized`) and only skip the *GUI-side dispatch* (`onCtrlChange*`) when the view isn't ready; when the view becomes ready, re-sync the GUI from stored param values (`getParamNormalized`) instead of replaying queued writes.

### B-03  Chunked processing corrupts output buffer pointers  · HIGH · **FIXED 2026-11-12** (reproduced and verified against the real code with `tests/note_off`, see fix log)
`DX10Processor::preProcess` (`mdaDX10Processor.cpp:1022-1030`): `ProcessData chunkData = data;` is a shallow copy; `chunkData.outputs` still points at the host's `AudioBusBuffers`. The loop writes `chunkData.outputs[0].channelBuffers32[i] = data.outputs[0].channelBuffers32[i] + currentSample;` which **modifies the host's channel pointer arrays**. From the 3rd chunk on, the offset accumulates (`P + c1 + c2` instead of `P + c2`), so audio is written to the wrong place and can run past the end of the host buffer; host pointers are also left shifted after the call.
**Test (pass 5):** I compiled the same loop with g++ against the real `ivstaudioprocessor.h` (256-sample block, event offsets 0,10,40,100): chunk writes land at offsets 0, 10, **50** (should be 40), **150** (should be 100), and the host's channel pointer is left at +150 after the call. The last chunk (156 samples) then writes up to sample 306 of a 256-sample buffer, i.e. ~50 floats past the end. `doProcessing` does write through `data.outputs[0].channelBuffers32` (`mdaDX10Processor.cpp:1159-1170`), so the real code does the same. What I did not test: whether a given host re-sets its channel pointers every block (many do, which would limit the drift to one block, but the overrun inside a block still happens). Triggered whenever a block has 3+ chunks, i.e. at least 2 distinct non-zero event offsets (a single chunk boundary is correct). Plausibly contributes to dropouts/buzz/crashes with dense MIDI.
Fix: keep local `float* outL/outR` pointers (or a local `AudioBusBuffers` + channel array) and never write through `data.outputs`.

### B-04  Missing null checks / stale events  · MED · **FIXED 2026-11-12** (reproduced and verified with `tests/note_off`, see fix log)
- `doProcessing` uses `data.processContext->tempo` unconditionally (`:1161`); `processContext` may be null in some hosts/validators.
- `preProcess` only calls `processEvents` if `inputEvents` is non-null; if null, `notes[]` from the previous block is replayed (the old code reset `notes[0] = EVENTS_DONE` at the end of each block; the new one doesn't).
(`notes[0]` IS initialised in `initialize()`, line 636.)
Fix: reset `notes[0]=EVENTS_DONE` after each block and when no events; guard `processContext`.

### B-05  Same-offset note-off/note-on gets re-ordered the wrong way  · MED · **FIXED 2026-11-12 together with B-18** (stable sort, note-off first at equal offsets)
`sortEventsByOffset` (`:1325-1354`): for equal offsets, if the first is a note-off and the second a note-on it **swaps them**, so the on comes first. The comment says "note-offs come after note-ons", which is wrong for a retrigger (old note off, then new note on, same pitch, same sample): the new note is started, then the note-off for that pitch releases it (the `noteToIDMap` now holds the new ID). (Effect corrected after pass 3, see B-18: the note-off is consumed *during* the retrigger fade and the note then restarts unreleased — a ghost/hanging note, not an immediately dead one.)
Verified: `DXGenerator::cutNote` defaults to `true` (`DXGenerator.h:389`); in that mode voices are keyed by pitch, a note-on for an existing pitch takes the `pushNote` retrigger path (`:370-377`, ramps down then re-triggers), and `noteOff(note)` (`:594-617`) puts that pitch's envelopes into release and sets `noteActive=false`. So an on→off order for the same pitch delivers the off while the retriggered voice is still fading (see B-18 for what happens next). Not yet confirmed audibly. In the default mode `noteID`/`noteToIDMap` are not used by the engine at all (`noteOff` ignores `noteID` when `cutNote` is true).
Also: the sort is a swap-based O(n²) pass that is not stable.
Fix: stable sort by offset only (`std::stable_sort` on a small struct array), or process in arrival order; note-off before note-on at same offset for the same pitch.

### B-06  Velocity is a coarse step function (integer division)  · MED · **FIXED 2026-11-12** (linear `v / 127` via `velocityGain()` in `DXGenerator.h`; see fix log)
`DXGenerator.cpp:340, 384, 442`: `voice.velocity = 0.01 * (100 / (127 / (*velocity)));` with `int32 velocity`. Both divisions are **integer** divisions. Result: only 20 distinct gains for 127 velocities, e.g. velocity 64–127 → 1.0 (all identical), 43–63 → 0.5, 32–42 → 0.33, 26–31 → 0.25, … velocity 1 → 0.0. `voice.velocity` scales the amp envelope (`:461`), so soft-to-medium playing is quantised and everything above ~half velocity is the same level.
Fix: `voice.velocity = (*velocity) / 127.0f;` (optionally a curve), in all three places.
Dropped from this item: my earlier claim that `processEvents`' `velocity*127` truncation loses velocities. Tested with float32: hosts sending `v/127.0f` round-trip exactly for all 127 values. Only a host using a different scale (e.g. v/128) would be off by one; the event's `noteId`/`channel` are ignored (low priority).

### B-07  Missing return + magic 173 in `BaseController::performEdit`  · MED · **FIXED 2026-11-12** (see fix log group 3)
`mdaBaseController.cpp:168-207`: the function only returns inside `if (tag < 173)`; for other tags it falls off the end (undefined behaviour in a non-void function). `173` is also hard-coded in begin/end edit and `setParamNormalized`.
Fix: return `kResultFalse` at the end; replace `173` with a named constant.

### B-08  State loading robustness  · MED · **FIXED 2026-11-12** (see fix log group 3; measured effect corrected there)
`BaseProcessor::setState` (`mdaBaseProcessor.cpp:407-439`):
- `for (i < temp, i < GPARAMS; ...)` uses the comma operator, so the `i < temp` test is ignored: always reads GPARAMS doubles even if the stream's count differs; any change to GPARAMS makes old sessions misread the bypass flag and everything after.
- `setCurrentProgram(temp)` is called with the value from the stream, no range check, indexes `programParams[20]`.
- `readDouble` result not checked.
Fix: loop `min(temp, GPARAMS)`, skip the remainder, range-check program, check stream returns. Consider adding a state version number.

### B-09  Instance-id filter compares a variable with itself  · LOW · **RESOLVED 2026-11-12** (dead filter removed, behaviour unchanged; see fix log group 3)
`DX10Processor::notify` (`:811`): `this->myInstanceId != myInstanceId` (the local read from the message is `myInstanceId_`). The condition is always false, so the filter never rejects anything. Probably harmless (connection points are per-instance), but the id plumbing (`this` pointer as int64) is then dead complexity. Decide: remove, or fix the typo.

### B-10  Real-time safety  · MED · **FIXED 2026-11-12** (heap use inside `process()` measured and now 0 in all test scenarios; see "B-10 heap measurement" and "Fix log: B-10" below; the paragraph under this heading is the original code-reading note)
On the audio thread: `setCurrentProgram` → 64 `allocateMessage/sendMessage` calls (34 env/LFO recalcs + 23 vertical switches + 7 toggles, `:928-967`); `noteToIDMap` (`unordered_map`) insert/erase per note (`:1055`, `:1068`) – and the engine ignores noteID in its default `cutNote` mode, so this is pure overhead; `DXGenerator::RVoicesMap[note]` (an `operator[]` map) can allocate on a first-seen pitch; `recalcLfoNRate` may be called per chunk. Heap allocation and locking in `process()` can glitch under load.
Fix: queue UI notifications via a lock-free queue and send from a non-audio context; replace the map with a fixed `int[128]`.

### B-11  `programParams` shared static filled in processor constructor  · MED · **FIXED 2026-11-12** (filled once by `DX10Processor::ensureFactoryPresets()`, called by the processor constructor and by the controller before it reads the table; see fix log)
`DX10Processor::programParams` is `static` and filled in `DX10Processor::DX10Processor()`. The **controller reads it directly** (`:163`, `:797`). If a host constructs the controller without ever constructing a processor (some scanners/validators/split-process setups), presets are all zeros; with multiple instances the ctor re-writes the same static concurrently. Also the processor's `setCurrentProgram(val)` and the controller's preset handler use the same table, but they are applied via **two independent paths** (host param → processor; host param → controller), so ordering/timing differences can leave GUI and DSP disagreeing (B-01/B-02).
Fix: make the preset table a `const` global initialised once (static initialiser or function-local static), not instance-constructed.

### B-12  Latent hardening issues in `processParameterChanges`  · LOW · **FIXED 2026-11-12 with B-52**
`:1878-1886`: `paramId < NPARAMS(100)` → `generators[paramId / 55]` but `GENCOUNT == 1`; IDs 55–99 would index `generators[1]` (out of range). IDs 55–99 aren't registered, so currently unreachable. `recalculators[1000]` is a member array that is **never default-filled** (only specific entries are assigned in `initialize()`), so any registered ID in 100..999 without an entry would call an indeterminate pointer. I checked by name pattern and found no registered param missing an entry (the 3 flagged names were aliases/removed entries, e.g. `MOD_GAIN_RESET` is commented out of the param list), so this is hardening, not a known crash. Fix: fill the array with a no-op in the constructor and bound-check.

### B-13  Tempo compare float vs double + uninitialised `bpm`  · MED · **FIXED 2026-11-12** (`bpm` init with B-04; compare + sync toggle in its own fix log, verified with `tests/note_off`)
- `Env::BPM` is `float` (`Env.h:20`), `bpm` is `double`. `recalcLfoNRate` stores `BPM = bpm` (rounded to float) and `doProcessing` compares `BPM != bpm` after every chunk. For a tempo not exactly representable as float (e.g. 133.3), the compare is always true, so `recalcLfoNRate` runs **every chunk**, and each call does `sendDoubleAttr(...)` = allocate + send an IMessage from the audio thread (`:1608`).
- `double bpm;` (`mdaDX10Processor.h:124`) has no initialiser, but `recalcLfoNRate` is called from `setState`/`setCurrentProgram` before any `process()`; with LFO tempo-sync on, `rate = bpm / quants[...]` uses garbage.
Fix: make `BPM` double, compare with a tolerance, initialise `bpm = 120`, and only send the rate message on real change.

### B-14  NVGView lifecycle  · MED · **CHECKED 2026-11-12**: the editor-memory part was B-54 (fixed), the null `baseController` in `removed()` is B-30 (fixed). `nvgViewReady = true` in the constructor is still early, but with the B-33 guards the handlers it enables are safe before `attached()`; `onNVGViewReady()` per frame only replays queued updates. Left as is.
`nvgViewReady = true` is set in the NVGView **constructor** (`NVGView.h:490`), before GL context/resources exist; `drawOpenGL` calls `onNVGViewReady()` every frame (`NVGView.cpp:1394`); `removed()` dereferences `baseController` without a null check (`:1140`) and clears containers but the commented-out section suggests cleanup of NanoVG context/shaders/textures may not be complete. Relevant to the 30 MB/open leak report.

### B-15  Registration scheme  · INFO
Password is a hard-coded string in the DLL and the registry value is its bcrypt hash; anyone who finds the string can generate a valid key; no per-user/machine data. If you intend this as real licensing, it needs per-license keys (signed). Also failure mode is total silence with no UI explanation except the "bad reg" view. Registry read is HKEY_CURRENT_USER, not HKLM (corrected pass 5; see B-45).

### B-16  Housekeeping  · INFO · **DONE 2026-11-12** (VST2 wrapper and `Source.cpp` out of the build, `BUILDING.md` replaces `HowToCompile_VS.txt`; dead files kept but not compiled)
`vst2wrapper.sdk.cpp` compiled into a VST3 target; `Source.cpp` empty; duplicate/backup sources; build instructions (`HowToCompile_VS.txt`) refer to VS2013/2015 and SDK 3.6.7.

## C. Pass 3 findings (voices / DSP) — read-only, not run

### B-17  Voices are never removed from the active list  · HIGH · **FIXED 2026-11-12** (reproduced and verified with `tests/note_off`, see fix log)
`DXGenerator::detectSilence` is the only thing that erases from `notes`, and its only call site is inside the commented-out block of the old `doProcessing` (`mdaDX10Processor.cpp:1089-1155`; the new one has `//generators[0].detectSilence(...)`). `notes` is only ever pushed to (`DXGenerator.cpp:361,365,387`). Consequences (by reading): every distinct pitch played stays in the list forever and is fully processed per sample (2 oscillators, filter, 3 LFOs, a log-scale lookup) even when silent, up to 128 voices; the `POLYCOUNT = 32` cap only triggers kills of the quietest voice but doesn't reduce load; a repeated pitch always takes the "retrigger" path instead of a fresh start (for a still-sounding voice that is a ~21 ms fade; for a finished voice the fade ends on the next sample). Possibly related to your note "silence detection proc in retrig note", but that note is ambiguous, so I am not claiming it is the same thing. Re-checked after review: searched the whole source tree incl. headers and backup files; the only `notes.erase` is in `detectSilence`, the only live call site is commented out, and `POLYCOUNT` is never changed from 32. Not measured: the actual CPU cost.
Fix direction: call a (fixed) silence detection after rendering, erase finished voices, and guard against the "repeat effect" comment in the old code.

### B-18  Note-off during the retrigger fade is lost → ghost / hanging note  · HIGH · **FIXED 2026-11-12** (reproduced and verified with `tests/note_off`, see below; still to be confirmed by ear in a host)
`noteOff` only edits envelope fields and sets `noteActive = false` (never read). If it arrives while the voice is `rampingDown` (retrigger fade, ~21 ms), `interpolateVoice` later calls `processNoteOn`, which re-initialises env/LFO state from the global attack/decay settings and starts a fresh note; nothing remembers that the key was already released. With decay on "hold" (`cdec = 1`) that note never ends. Triggered by: any note-off within ~21 ms of a same-pitch note-on (fast repeated notes) and by the B-05 reordering (off arrives right after the on). Checked after review: `noteActive` is never read anywhere (whole tree). I also modelled the envelope-1 logic in Python exactly as I read it (`retriggerVoice`, `noteOff`, `interpolateVoice`, `processNoteOn`; this tests my reading, not the real code). Result: "on then off" (the order B-05 produces) and "off then on" give **identical** output, i.e. the note-off is ignored. With decay on hold the voice then sustains at full level forever (a plain note-off alone decays to 0 in ~0.2 s); with normal decay it plays a spurious note (level 0.72 at +50 ms vs 0.007 for a real release). Caveats: it needs a same-pitch note-off within ~21 ms after a note-on, which in practice means the B-05 same-sample case (host dependent) or extremely short notes; and this is a *ghost/hanging note*, which is not the same symptom as your "second voice drops out" — that symptom is not explained yet.
Fix direction: keep a "released" flag per voice and honour it in `processNoteOn`/ramp end.

### B-19  (WITHDRAWN) "Retrigger fade ends with a click because `cenv` lags `env`"
My first version assumed `cenv` lags `env` during the fade. Wrong: `retriggerVoice` sets `catt = 1.0` (`DXGenerator.cpp:405`), so `cenv` follows `env` with a one-sample lag, and my model showed no step in the level at fade end. Withdrawn. What remains true but is probably masked: envs 2/3 (modulation) are re-initialised (`env = 1, cenv = 0`) when the fade ends, while the amp is already near zero. "Modulation drops out on retrig when env too high" therefore has **no explanation yet**.

### B-20  `recalcWidth` writes the wrong variable (copy-paste)  · MED · **FIXED 2026-11-12** (see fix log B-20 + B-22 + B-47)
`DXGenerator.cpp:1058-1062`: the main oscillator's width handler sets `modWidthPole`/`modWidthEdge` (the *modulator's*), while the main oscillator reads `widthPole` (`:868`), which is never assigned anywhere (stays 1.0). Intent is documented: `DXGenerator.h:162` says "widthPole is set on width recalc to represent width level sign", and no live code assigns it (whole-source grep). Effects: the main width env polarity logic is disabled, and the modulator's polarity is overwritten by the main WIDTH knob (`WIDTH_ < 0.5` flips it) until `recalcModWidth` runs again.

### B-21  TRI waveshape has a wrap discontinuity and exceeds ±1 when width ≠ 0.5  · HIGH · **FIXED 2026-11-12** (your choice: keep the morph below 0.5, mirror it above 0.5, anti-alias the edge; see fix log) · whether this was *the* "buzz from waveshape mod" is still unconfirmed by ear
`QBLimitedOscillator::doTriangle2` (`:163-194`): `out = 4*tri - 1` with `t = modulo*(width+0.5)`. I evaluated it numerically: width 0.5 → continuous triangle in [-1,1]; width 0.3/0.7 → jump of ∓0.8 at the wrap; width 0.1/0.9 → jump of ∓1.6, and for width > 0.5 the minimum goes to -1.8 .. -2.6 (outside ±1; level and DC change with width). The BLEP call is commented out (`:192`), so the jump is unfiltered (aliasing). Modulating width with an env/LFO changes the jump size continuously, which fits a "buzz from waveshape mod". The TRI jump analysis applies to TRI only; SQUARE also uses width, as pulse width (corrected pass 5). Review notes: the phase is wrapped to [0,1) (`checkWrapIndex`), so the numbers above hold for the code as written. **Your todo says "buzz from waveshape mod", which is ambiguous**: I assumed width modulation, but it could mean the modulator's waveform; the modulator (`WTOsc2`) is the same oscillator class with the same width logic, so the same jump/overshoot applies to it when TRI is selected. That this is *the* buzz is still a hypothesis. A jump at the wrap may also be the designed "morph to saw" character, only the missing anti-aliasing is clearly a defect.

### B-22  Oscillator-1 note index is not clamped; `midiFreqTable` has 128 entries  · MED · **FIXED 2026-11-12** (reproduced: MIDI 127 with octave +2 / interval +12 played at 20 Hz; see fix log B-20 + B-22 + B-47)
`DXGenerator.cpp:848-850`: `newNote = note + newInt + (newOct-3)*12`, range about **-37 .. 176**, then `midiFreqTable[newNote]` (`lookuptables.h:4`, `[128]`). The modulator clamps to 1..109 (`:874`); oscillator 1 does not. Reachability (corrected after review): the index only leaves 0..127 with extreme settings, e.g. octave 5 (+24) with notes from about MIDI 104 (less with a positive interval or env), or very low notes with octave 1 and a negative env; default settings stay in range. The range above assumes the interval/octave step lists as I read them (not checked in the GUI). When it does overflow, `COscillator::update` clamps the frequency to ±20480 Hz (`Oscillator.h:309-314`), so the likely result is a wrong/ultrasonic pitch rather than a crash, unless the garbage float is NaN (comparisons then don't clamp; unverified).

### B-23  Out-of-range write into `ctrlValues[NPARAMS]` from the toggle round-trip  · **HIGH** · **FIXED 2026-11-12** (measured: much worse than "minor"; see fix log B-23 + B-55)
`setCurrentProgram` and `setState` send `PMSG_STEP_SWITCH2` for every id in `toggleBtnPids` = `{50, 113, 118, 123, 128, 133, 138}` (`constants.h:495`). The controller replies with `PMSG_STEP_SWITCH` and `DX10Processor::notify` does `generators[0].ctrlValues[pid] = value` with no bounds check (`mdaDX10Processor.cpp:818-822`); `ctrlValues` is `int[NPARAMS=100]` (`DXGenerator.h:198`), so pids 113-138 write past the end. The written value is the toggle state, int 0 or 1.
**Pass 5 layout** (g++, real `DXGenerator.h` with scratch-copy compat patches; relative offsets only depend on the following int/float members so they should match MSVC): `ctrlValues` +996, 400 bytes; then 4 ints (+1396), `voiceEnvAtt/Dec/Rel` (+1412), `paramInc/Thresh` (+1424), `rampedParams[8]` from +1432, each `ParamRampObj` = 8 floats (32 bytes: lastTarg, difference, targ, inc, level, thresh, zeroToTargTime, lastTargToNewTime). So, with `rampedList = {WIDTH_, GAIN_, FILTER_, MOD_GAIN_, ...}` (assuming the `*_RAMPINDEX` enums follow that order, not checked):
| pid | lands on |
|---|---|
| 113 | `rampedParams[0]` (WIDTH_) `.level` |
| 118 | `rampedParams[1]` (GAIN_) `.difference` (unused) |
| 123 | `rampedParams[1]` `.zeroToTargTime` (unused) |
| 128 | `rampedParams[2]` (FILTER_) `.inc` |
| 133 | `rampedParams[3]` (MOD_GAIN_) `.lastTarg` (unused) |
| 138 | `rampedParams[3]` `.thresh` |
Int 0/1 reinterpreted as float is 0 or 1.4e-45 (about zero). Consequences from reading `ParamRampObj` (not heard): width level is forced to ~0 right after a preset/state load and glides back; FILTER `.inc` ~0 freezes that ramp, but `setLevels2` recomputes `inc` on the next filter change and `initLevel` has already put level on target, so likely harmless; **MOD_GAIN `.thresh` ~0 stays that way** (only set at construction), which removes the `inc` clamp and the final snap, so the MOD_GAIN smoother may jitter by up to its step (~0.0002) around the target forever. Probably minor, not the cause of big failures, but it is a real corruption and a data race with the audio thread. Downgraded from HIGH. Fix: bounds-check, or size `ctrlValues` to GPARAMS.

### B-24  `RVoicesMap` is never freed  · LOW · **FIXED 2026-11-12** (see fix log group 3)
`new RVoice[PARTVOICES]` in `DXGenerator::init()`; `~DXGenerator` does not `delete[]` it (it frees `filterLogScale2`, `m_pFilterLogParam`, `ddMap`, `doRecalc`). One leak of 128 `RVoice` objects per plugin instance creation.

### B-25  Smaller dead / copy-paste items in the voice code  · LOW · **MOSTLY FIXED 2026-11-12** (pan smoothing = B-55; index typos fixed; see fix log group 3)
- `interpolateVoice` `:688-689`: LFO 2/3 amplitude ramps use LFO 1's amplitude (`[4]`); and `saveTargValues` (`:416-418`, `:422-424`) writes the ramp step into `m_dAmplitude` instead of `envInc` for LFOs 4-6. Harmless today because `singleVoiceProcessing` resets `m_dAmplitude` from the global level every sample, and killed voices don't render LFOs.
- `DXGenerator::init` `:98`: `doRecalc[FINE_ENV_SRC] = &recalcFineEnv` (should be `recalcFineEnvSrc`). Harmless: `controls[FINE_ENV_SRC]` is never read (voices read `ctrlValues` directly).
- Pan smoothing in `singleVoiceProcessing`/`deadVoiceProcessing` is a no-op (`prePan = pan` then `panDiff = pan - prePan`).
- `deadVoiceProcessing` uses a stale `gainerTemp` and omits modulator mix/`MOD_OSC_GAIN`, so killed-voice fades sound slightly different from live voices.
- `carPitchMultiplier` is assigned in several recalc functions but is never read anywhere in the live source (grep of the non-backup files). The `controls[INT_]`/`controls[MOD_INT_]` writes next to it were not checked for readers.

### B-26  Performance notes  · INFO
Per voice per sample: `m_pFilterLogParam->toPlain` (your own comment says it's heavy; a precomputed table `ddMap` exists but is commented out), `std::deque`/`RVoicesMap[]` indexed per sample, pointer-to-member `modChoice` call, float/double mixing. Combined with B-17 this is a plausible main CPU cost, but nothing has been profiled.

### B-27  Message IDs are pointers to single `char` constants, not C strings  · LOW · **FIXED 2026-11-12** (processor side with B-10; controller side via `msgIdStr()` in `constants.h`; see fix log. See also B-53.)
`constants.h:230-252`: e.g. `const char PMSG_STEP_SWITCH_CHAR = '3';` and the code calls `msg->setMessageID(&PMSG_STEP_SWITCH_CHAR)`. `setMessageID` expects a NUL-terminated string; the host's `IMessage` implementation will typically `strlen`/copy it, reading past the one-byte constant into whatever follows in read-only memory. It works because the receivers only look at the first character (`*message->getMessageID()`), but it is undefined behaviour and the stored ID string is arbitrary. Also `BaseController::notify` first does `strcmp(getMessageID(), "InstanceID")` on these. Fix: use real string literals (and compare with `strcmp`) or `const char X[] = "3"`.

## D. Pass 4 findings (GUI lifecycle) — read-only, not run

### B-28  `NVGView::removed()` deletes the NanoVG GL context without making the GL context current  · MED · **FIXED 2026-11-12** (context made current around `nvgDeleteGL3`). Not the cause of the memory growth: measured, it made no difference with one instance (the GL context is destroyed on close anyway); it matters with several editors open. The memory growth was B-54.
`NVGView.cpp:1137-1178`: `nvgDeleteGL3(vg)` issues `glDelete*` calls (programs, buffers, textures, the font atlas, and the images made by `nvgCreateImageMem`). The only places that call `lockContext()/makeContextCurrent()` are `platformOpenGLViewCreated` and `platformOpenGLViewSizeChanged` (grep). `removed()` runs before `COpenGLView::removed`, but nothing there guarantees the thread's current context is this view's. If it isn't, the deletes go to the wrong context or nowhere and the GPU/driver memory stays allocated. Not measured; confirm with a memory counter across open/close cycles. Also: `images[]` ids are never passed to `nvgDeleteImage`, they rely on `nvgDeleteGL3`.

### B-29  `DX10Controller::nvgViewController` is a raw pointer that is never cleared  · MED · **FIXED 2026-11-12** (confirmed with `tests/editor_cycle`: after closing the editor it pointed to a freed `NVGViewController`; now cleared from `~NVGViewController`)
`mdaDX10Controller.cpp:83`: set on every `createSubController`; nothing nulls it when the editor closes. `~NVGViewController` does not delete `nvgView` (commented out) and who releases the controller/view depends on VSTGUI's ref-counting (not traced). Handlers at `mdaDX10Controller.cpp:358-466` call `nvgViewController->nvgView->...` with no null check and no view-ready check of their own; they are protected only by callers checking `nvgViewReady`. If any path reaches them with the editor closed, it dereferences a freed object. Also `NVGViewController::isCreated` is just `iController != nullptr`.

### B-30  Editor open/close ordering hazards in `NVGView`  · LOW-MED · **FIXED 2026-11-12** (null `vg` / `baseController` / `selectedStepTarg` / OCT_ view guarded; see fix log "try/catch review"). The redraw-every-frame point (`glFinish` + `invalid`) is unchanged.
- `removed()` sets `vg = nullptr`, but `drawOpenGL()` (`:1391`) calls `nvgBeginFrame(vg, ...)` unguarded; a draw between `removed` and destruction would pass a null context.
- `removed()` dereferences `baseController` with no null check (see also earlier note in the doc 01).
- `attached()` does `selectedStepTarg = viewIndex[OCT_]; selectedStepTarg->RSBType = 1;` (`:1097`): null deref if the uidesc has no view with that tag.
- `attached()` clears `controlMap` but `removed()` does not clear `RViewsMap`, `knobMap`, `buttonMap`, etc. (that block is commented out). Today harmless because the whole `NVGView` is recreated per editor open (`createSubController` makes a new `NVGViewController`/`NVGView`), but if that ever changes, `map::emplace` would keep the stale first entry.
- `drawOpenGL` calls `glFinish()` and `invalid()` every frame: the view redraws continuously while open (CPU/GPU load, and a bar on the idle-cost story).

### B-31  Custom `CView` fields are uninitialised  · LOW-MED · **FIXED 2026-11-12** (SDK edit, `cview.h`, logged in 05-sdk-mods)
See `05-sdk-mods.md` (pass 4). `customRSType` and `customRSDisplayActive` have no initialisers; set only in `uiviewfactory.cpp:150-190` when a name attribute starting `#` exists or when none exists. Views from other paths hold garbage. Currently masked by `customRSViewName.empty()` checks in `NVGView::attached`.

### B-32  `std::stoi` on the `sub-controller` attribute can throw  · LOW · **FIXED 2026-11-12** (SDK edit, `uiviewfactory.cpp` uses `strtol`, logged in 05-sdk-mods)
`uiviewfactory.cpp:170`. Affects any `#`-named view whose `sub-controller` is not a number.

### B-33  GUI handlers that can dereference a missing view, hidden behind `try/catch(std::exception)`  · LOW-MED · **FIXED 2026-11-12** (reproduced: crash despite the try/catch; see fix log "try/catch review")
- `NVGView::onStepCtrlSwitch` (`NVGView.cpp:321`): `selectedStepTarg = viewIndex[pid]; selectedStepTarg->RSBType = ...` with no null check and no `viewsReady` check. `viewIndex[]` is null-initialised, so a `pid` with no registered knob/button/vert-switch/step-button is a null dereference. A null deref is not a C++ exception, so the surrounding `try/catch` does nothing. The comment at `:36` ("why is pid 153 being thrown ... no viewindex at 153!") shows this class of problem has been seen. `initResetWidthAnim` (`:38`) does guard with `viewsReady[pid]` (initialised false in the ctor, `NVGView.h:487`), but sliders never set `viewsReady`.
- `stepCtrlSelectAnim.controlIdx != NULL` (`:325`) treats pid 0 as "none"; param 0 is a real param, so its highlight is not cleared.
- Draw handlers use `RViewsMap.at(name)` / `knobMap.at(name)` (`drawKnob` `:2234`, `drawKnob2`): `std::out_of_range` is not caught anywhere in `drawOpenGL`, so a draw handler registered without a matching map entry would terminate the host. All current setup paths insert into both, so this is latent.
- All the `try { } catch (const std::exception&) { }` blocks swallow errors silently; none of them can catch the null-deref/OOB cases above.

### B-34  `NVGView::valueChanged` has the reset index sign reversed  · LOW · **FIXED 2026-11-12** (delegates to `onResetButton`)
`NVGView.cpp:438-441` computes `idx = WIDTH_RESET - pid` (negative for pids above WIDTH_RESET) and indexes `resetSwitch[idx]`, while `onResetButton` uses `pid - WIDTH_RESET`. The method is marked "wtf" in the source; `NVGView` is declared an `IControlListener`, but the only `setListener(this)` is commented out (`:474`), so it is likely never called. Unverified.

### B-35  GUI text effect and animations advance per drawn frame, not per unit of time  · INFO
`RandyTxt::processText/processTextRev` add `0.01`/`0.02` to their timers each call, and `drawOpenGL` redraws continuously (`glFinish(); invalid();`). Animation speed therefore depends on how fast the host/driver calls `drawOpenGL`. Also `getViewAttr` (`:18`) does `new char[]` per call and never frees, but nothing calls it (dead).

### B-36  The `revent` headers that actually compile live outside the project and the SDK  · MED (reproducibility) · **FIXED 2026-11-12** (revent now at `VST3_SDK/external/revent`, GLEW + libbcrypt vendored in `external/`; see `external/README.md`)
- `NVGView.cpp:4` / `NVGView.h:23` include `../SFML-2.5.1-64/include/SFML/revent/REvents.h`, and the x64 configs add `C:\D\SFML-2.5.1-64\include\SFML\revent` (and `C:\D\KHR`, `C:\Users\PC\libbcrypt\include`) to the include path. On this machine `C:\D\SFML-2.5.1-64\...\revent` exists (newest edits 2023-2025), `C:\D\KHR` and `C:\Users\PC` do **not**; another copy is in `C:\vst3sdk\VST3_SDK\build\SFML-2.5.1-64\...` (2019-2022), and a third in `<project>\revent\` (2019, stale: `REvents.h` 41 lines different, `REvent.h` 216, `REventable.h` 41, `REventBinder.h` 18).
- Consequence: the event/data system your GUI depends on is not in the project folder, so a backup/git of the project alone cannot build it, and which copy gets used depends on the configuration's include order. I have **not** verified which copy the last successful build used; the `C:\D` copy is the likeliest (newest, referenced by absolute path). Treat the docs' description of `revent` below as based on the in-project copy (stale) where they differ.
- Some `<project>\revent` code that the C:\D version has commented out (the `RVoice` array / `boundProcessors` parts of `REvents`) is therefore probably dead in the real build.

### B-37  Per-frame copies in the draw dispatcher  · LOW (perf) · CONFIRMED in code (copy present in both revent versions; cost unmeasured)
`REvents::execDataHandler` (`REvents.h`) does `RData rd = RDataMap.at(...)` (copy by value, includes strings) for every registered control, every frame, then `boundHandlers[name]` (a map `operator[]`, which can insert). The draw functions then ignore the copy and look up `RViewsMap.at(reIn.dataName)` again. `drawEnv1-6` also do 4-5 string-keyed `knobMap.at("env1Att")`-style lookups each frame.

### B-38  Hard-coded view names in the per-frame draw functions, uncaught `.at()`  · MED · **MITIGATED 2026-11-12** (one exception boundary in `drawOpenGL`: a failed lookup skips the frame instead of escaping into the host; the lookups themselves are unchanged)
`drawEnv1..6` (`NVGView.cpp:1467-1930`) look up `"env1Att"`, `"env1DecToggle"`, `"lfo1Amp"`, `"lfo1Type"`, `"lfo1PhaseSlider"`, ... with `.at()`. If the uidesc ever drops or renames one of those views, `std::out_of_range` escapes `drawOpenGL` into the host. The envelope shapes drawn are a **visual approximation** from the knob positions (not computed from the real DSP coefficients), and `drawEnv1-3` fill after `nvgClosePath` without a new path.

### B-39  `data.processContext` is dereferenced without a null/valid check  · MED · **FIXED 2026-11-12 with B-04**
`DX10Processor::doProcessing` (`mdaDX10Processor.cpp:1161`): `bpm = data.processContext->tempo;`. VST3 hosts may pass a null `processContext` (and `tempo` is only valid if `state & kTempoValid`). Null = crash on the first block; invalid tempo = garbage/zero `bpm` feeding LFO sync (see B-13). It also runs once per chunk.

## E. Pass 5 findings (filter / numerics) — read-only

### B-40  No denormal protection; old voices decay into denormals forever  · MED · HYPOTHESIS (code reading; not measured)
No `_MM_SET_FLUSH_ZERO_MODE`, `_controlfp`, or denormal guard exists in the live source (grep). Envelope state is float and decays as `env *= cdec` with no floor (B-17: voices never leave `notes`), so e.g. with a mid decay setting (`cdec ~ exp(-4.5e-4)` at 44.1 kHz) `env` falls below ~1e-38 after ~4-5 s and stays denormal; `cenv` follows. On x86 without FTZ/DAZ, denormal float arithmetic costs tens to hundreds of cycles. Combined with up to 128 retained voices this could cause CPU growth and dropouts the longer you play. Some hosts set FTZ/DAZ on the audio thread, which would hide it. Check: profile with many distinct pitches played then released, or set FTZ/DAZ and compare.

### B-41  Filter Q knob only spans a fraction of the SVF's Q range  · INFO
`MIN_FILTER_Q=1.0`, `MAX_FILTER_Q=2.5` (`SynthParamLimits.h:120-121`) are mapped through `setQControl`, whose comment expects 1..10 (Q 0.5..25). Result: Q 0.5..4.6. Possibly intentional (tame resonance), possibly a leftover.

### B-42  Filter cutoff is not clamped to Nyquist  · LOW · CONFIRMED in code, only matters at low sample rates
`update()` uses `tan(pi*fc/sr)` with fc up to 18 kHz. At sr < 36 kHz (e.g. 32 or 22.05 kHz sessions) fc can exceed Nyquist, `g` goes negative/huge and the filter blows up. Also `update()` evaluates `tan` per voice per sample (B-26).

### B-43  LFO saw "anti-click chase" does not smooth the wrap and mixes units  · LOW · CONFIRMED in code (not heard)
`LFO.h:30-52` `doSaw`: on wrap it sets `interimModulo = .9999` and a per-sample step `sawModuloInc = 1000*(interim - modulo)/sawSmooth/sr` (so `sawSmooth = 25` is meant as **milliseconds**), but the chase ends when `chaseModuloStep >= sawSmooth`, which counts **samples** (25 samples = 0.6 ms at 44.1 kHz). Also the chase output starts at about +1, which is the same value the normal path gives right after the wrap (`-1*unipolarToBipolar(m)` = about +1), while the value just before the wrap is about -1. So the full -1 -> +1 jump still happens at every wrap; the chase only holds the output near +1 for 25 samples and then hands over with a small step (~0.05). Net effect: the saw LFO is still a hard sawtooth, as a saw LFO inherently is; the code does not do what its comment intends. Only matters if a saw LFO on gain/filter produces audible clicks.

### B-44  Phase accumulators in the oscillator/LFO base class are 32-bit float  · INFO
`CLFOscillator.h`: `m_dModulo`, `m_dInc`, `m_dFo`, `m_dOscFo` are `float`. Quantisation of `inc` near modulo ~1 is about 6e-8, i.e. ~0.13% rate error at 1 Hz and ~0.5% at 0.25 Hz LFO rates (negligible for audio-rate oscillators). Matters only for tempo-synced slow LFOs drifting against the host tempo.

## F. Registration (pass 5)

### B-45  Registration lives in HKEY_CURRENT_USER and is checked only once, in the processor constructor  · INFO / support risk · CONFIRMED in code
`regReader.h` opens `HKEY_CURRENT_USER` (function is named `...HKLM`). A host or service running as a different Windows user (or a different profile/roaming setup) will not see the key, and the synth is silently mute (`noteOn` is the only gate) apart from the "No registration detected" overlay if the editor is opened. The check runs once per processor instance; a bcrypt hash at the stored cost factor is computed on every instantiation (also during plugin scanning), which costs time on the constructing thread. Registering while a project is open needs the plugin to be reloaded. The GUI copies `constants::isRegLegit` when the editor is created (not live), and the static is set by the *processor*; if a host ever creates the controller/editor before (or without) a processor in the same process, the overlay shows even for a registered user.

### B-46  Protection is weak by design  · INFO
The shared secret is a plain string in the DLL and the stored value is just its bcrypt hash, so the same registry value works for every user and anyone who extracts the string can create a valid hash (`HKCU` is user-writable). Nothing is per-user, per-machine or time-limited. Fine if the goal is only to nudge honest users; not a licence enforcement. Unregistered behaviour is total silence rather than a demo mode, which also looks like a bug to a user who skipped registration.

### B-47  Changing the modulation type while a note is sounding leaves stale modulation on the carrier  · LOW-MED · **FIXED 2026-11-12** (reproduced: 77.8 Hz detune stuck after freq-mod -> none; see fix log B-20 + B-22 + B-47)
`doPhaseMod` writes only `WTOsc.m_dPhaseMod`; `doFreqMod`/`doDetuneMod` write only `WTOsc.m_dFoModLin`; `doNoMod` writes neither (`DXGenerator.cpp:1011-1042`). `update()` adds `m_dFoModLin` (Hz) to the frequency and `doOscillate` adds `m_dPhaseMod` to the phase (`QBLimitedOscillator.cpp:426`). If you switch `MOD_TYPE_` from Freq/Detune to None/Phase on a sounding or held voice, `m_dFoModLin` stays at its last value (up to a few hundred Hz at high mod depth) for the rest of that note, i.e. a constant out-of-tune offset; switching away from Phase freezes a constant phase offset (inaudible apart from a click). New notes are not affected because `processNoteOn` -> `startOscillator()` -> `reset()` zeroes both. Possibly related to "INIT modulation" impressions after changing the mod type mid-note; not a match for "adds up on voice stealing". Fix idea (not applied): zero the unused field in each of the mod functions.

### Pass 5 refinement of B-05 / B-18
See `06-note-event-trace.md`. B-05 (same-sample reorder) is mostly masked by B-18 and is harmless for a held key. B-18 requires: same pitch played before and still audible, plus a gate shorter than ~21 ms; with decay at hold the note then never ends (model). Both are still unreproduced in a host.

## G. Found during build cleanup (2026-11-12)

### B-48  The VST2 entry point wraps an unregistered class  · MED · **RESOLVED 2026-11-12: VST2 removed** (no `VSTPluginMain` export, wrapper and dead VSTSynth*/Voice/RSSynth002Voice/Source.cpp no longer compiled)
The `.def` exports `VSTPluginMain` (from the SDK's `vst2wrapper.sdk.cpp`, compiled into the plugin). Its `createEffectInstance` lives in the dead `VSTSynthProcessor.cpp:14` and calls `Vst2Wrapper::create(GetPluginFactory(), Steinberg::Vst::rs::Processor::cid, 'DXS0', ...)`. `rs::Processor` (the VSTSynth class) is **not** registered in `factory.cpp`, so a VST2 host loading the DLL would get no plugin. Either remove the VST2 export or point it at `DX10Processor::uid`.

### B-49  Link used `/FORCE:MULTIPLE` to hide duplicate definitions  · FIXED 2026-11-12
`RSVERSION` (`constants.h:19`, non-const pointer defined in a header -> one definition per TU) and `MoreInfoButton::onMouseEntered` (defined out-of-class in `NVGView.h` without `inline`). Fixed (`const char* const`, `inline`) and `ForceFileOutput` removed from Debug|x64 and Release|x64.

### B-50  Release|x64 compiles with C++ exceptions disabled while the code uses try/catch  · MED · **FIXED 2026-11-12** (Release now uses the default /EHsc; C4530 gone)
`<ExceptionHandling>false</ExceptionHandling>` in Release|x64; the compiler warns C4530 ("C++ exception handler used, but unwind semantics are not enabled") for files that use `try/catch` (registry read, GUI handlers) and STL. With /EHs- an exception thrown (e.g. by `std::stoi`, `map::at`, `std::wstring_convert`, the registry reader) is not reliably caught and destructors are not run during unwinding. Debug|x64 uses the default (/EHsc). Should be /EHsc in both.

### B-51  `CBMoreInfoButton::onMouseExited/onMouseEntered` do not return a value on every path  · LOW · **FIXED 2026-11-12** (SDK edit, returns `kMouseEventNotHandled`; logged in 05-sdk-mods)
Undefined behaviour for a non-void function; SDK mod file.

### Fix log: B-18 + B-05 (2026-11-12)
- `DXGenerator::noteOff`: if the voice is in its retrigger fade (`rampingDown`), set `RVoice::releasePending` instead of touching the envelopes (ignored for a voice being stolen); otherwise `releaseVoice()`. The release logic moved unchanged into `DXGenerator::releaseVoice(RVoice&)`.
- `DXGenerator::interpolateVoice`: after the fade restarts the note (`processNoteOn`), apply a pending release.
- `DXGenerator::retriggerVoice`: clears `releasePending` (a new note-on/steal supersedes it). `RVoice::killIt` now initialised.
- `DX10Processor::sortEventsByOffset`: stable insertion sort by (offset, note-off before note-on). Needed together with the B-18 fix: otherwise the old note's note-off at the same sample would now release the new note.
- Evidence: `tests/note_off/build_and_run.bat` drives the real `DX10Processor::process()` with VST3 event lists (12 cases, decay HOLD and 0.6). Old code: 2 failures (note-off 10 ms into the fade -> level 0.9985 of peak at 2.5 s; 25 fast repeats with 15 ms gates -> 0.8418 at 3.0 s). Fixed code: 0 failures. Not yet heard in a host.

### Fix log: B-03 (2026-11-12)
- `DX10Processor::preProcess`: each chunk gets its own `AudioBusBuffers` and local channel-pointer array (offset by the chunk start), so the host's `data.outputs[0].channelBuffers32` is never written. Also skips rendering unless output bus 0 exists with at least 2 channels (`doProcessing` writes two channels).
- Evidence: the test no longer resets channel pointers between blocks; it checks after every block that the host pointers are unchanged and that a guard zone after each buffer is untouched, and adds a dense-MIDI case (note every 2 ms, 4 pitches). Old code: host pointers moved after 258 blocks, 41852 samples written past the buffer end. Fixed code: 0 and 0, all 14 note cases pass. Debug and Release build with 0 errors. Not yet heard in a host.

### Fix log: B-04 + B-39, part of B-13 (2026-11-12)
- `DX10Processor::preProcess`: `notes[0] = EVENTS_DONE` at the start of every block (a block without an event list no longer replays the previous block's events) and at the end. Events with an offset at/after the block end are applied after the last chunk instead of being skipped. Blocks that render nothing (zero samples, bypassed, no stereo output) still apply their notes, so a note-off is not lost.
- Per-event handling moved unchanged into `DX10Processor::dispatchNoteEvent(note, velocity)`.
- `DX10Processor::doProcessing`: tempo is only read if `processContext` is non-null, `kTempoValid` is set and tempo > 0; otherwise the last tempo is kept. `bpm` (processor and `DXGenerator`) initialised to 120.
- Evidence: 4 new cases in `tests/note_off` (21 note cases total). Old code: no event list on empty blocks -> held note's minimum level 0.0002 of peak (note-on replayed every block, constant retrigger; a normally held note, the new control case, measures 0.4763); note-off with offset past block end -> hanging note at 0.9198 of peak; null `processContext` -> access violation (0xC0000005). Fixed code: 0 failures. Debug and Release build with 0 errors.
- Seen while reading, not changed (B-04 fix): the overflow branch of `processEvents` checks `npos < EVENTBUFFER - 1` before writing 3 slots, which can reach index `EVENTBUFFER`; harmless because `notes` has 8 spare slots (`notes[EVENTBUFFER + 8]`).

### Fix log: B-13 rest (2026-11-12)
- `Env::BPM` is now `double` (same type as `bpm`), so the tempo compare in `doProcessing` is only true on a real tempo change.
- `doProcessing`: on a tempo change, an LFO with sync off only records the new tempo (its rate does not depend on tempo); only synced LFOs recompute their rate (and send the rate message).
- `recalcLfo1/2/3BPM` (sync toggle) now recompute the rate. Before, switching sync on or off only took effect at the next rate-knob move or tempo change; at tempos a float cannot hold exactly it was masked by the per-chunk recompute.
- Test harness: the processor now gets a real host context (`HostApplication` from the SDK hosting sources) and a peer that counts messages sent from inside `process()`. 5 new B-13 cases. Old code: 133.3 BPM -> 807 audio-thread messages in 3.1 s (sync off as well as on); sync switched on at 0.5 s at 120 BPM -> LFO 1 rate stays at the free value 10.04 instead of 4.0. Fixed: 0-3 messages per render (initial tempo adoption and the parameter changes themselves), all rates as expected including a 120 -> 140 tempo change. All earlier cases still pass. Debug and Release build with 0 errors.
- Still open (B-10): `recalcLfoNRate` still allocates and sends an IMessage from the audio thread, now only on real rate changes (knob, sync toggle, tempo change with sync on). A host tempo ramp with sync on still sends one per synced LFO per block.
- Seen while reading, not changed (B-13 fix): `recalcLfo2Rate`/`recalcLfo3Rate` set `mainEnvLfo[5/6].m_dOscFo` from `EnvLfoCtrls[2]/[3].rate` in the sync-off branch (wrong index), but the line after overwrites it with the right value, so it has no effect.

### B-10 heap measurement (2026-11-12, discovery)
Method: `tests/note_off` replaces global `operator new/delete` in the test exe (the plugin objects and SDK libs are linked in, so their allocations go through it), records every allocation/free made inside `process()` with its call stack, and names the functions via DbgHelp. The processor has a real `HostApplication` context, so `allocateMessage` allocates like a host would (a real host's allocation is in the host, but it is still a heap allocation on the audio thread). Not seen: direct `malloc`, aligned `new`, allocations inside the host. No locks (mutex/critical section/FLock) exist in the processor/engine sources.

| Scenario | allocs | frees | Where |
|---|---|---|---|
| idle, no notes (1 s) | 0 | 0 | - |
| one held note | 3 | 0 | `notes` deque growth (2) + `noteToIDMap` insert (1) |
| 4-note arp, 60 notes | 62 | 60 | `noteToIDMap` insert/erase: 1 alloc + 1 free **per note**; deque growth 2 |
| 60 distinct pitches | 69 | 60 | `noteToIDMap` per note; deque growth 9 (first time only, MSVC deque blocks are reused) |
| sweep of params 0-54, 110-143 | 59 | 56 | GUI messages (`sendDoubleAttr` LFO rates, `sendIntAttr`): ~8 allocs per message |
| 7 preset changes | 4835 | 4832 | `setCurrentProgram` on the audio thread: per preset ~64 `sendPidVal` + LFO-rate `sendDoubleAttr` messages, ~690 allocs per preset |
| LFO 1 synced, tempo 120 -> 140 | 27 | 24 | 3 LFO-rate messages |

Corrections to the note above: `RVoicesMap` is a plain preallocated array (`RVoice*`), not a map; it never allocates. `recalcLfoNRate` is no longer called per chunk (B-13 fixed).
Remaining sources: (1) `noteToIDMap` (`unordered_map`) in `dispatchNoteEvent`, unused in `cutNote` mode; (2) the `notes` render list (`std::deque`) in `DXGenerator::noteOn`/`detectSilence`; (3) every `sendMessage` from the audio thread (`sendPidVal`, `sendDoubleAttr`, `sendIntAttr`). SDK 3.6.12 has no `DataExchange` API; it does have `OneReaderOneWriter::RingBuffer` (preallocated) and `Steinberg::Timer` (Windows: `SetTimer` on the creating thread, i.e. the host's main thread).

### Fix log: B-10 (2026-11-12)
- Step 1: `DX10Processor::noteToIDMap` (`std::unordered_map`) replaced by `int32 noteToID[128]` (-1 = none), same lookups.
- Step 2: `DXGenerator::notes` (`std::deque<int32>`) replaced by `VoiceList` (fixed 128 entries, order-preserving `eraseAt`).
- Step 3: GUI messages. `BaseProcessor::sendDoubleAttr/sendPidVal/sendIntAttr` build a small `QueuedMsg`. Inside `process()` (thread-local flag set by `BaseProcessor::process`) it is pushed to a preallocated `OneReaderOneWriter::RingBuffer` (4096 entries, SDK); a `Steinberg::Timer` (15 ms) created in `BaseProcessor::initialize` (host main thread) drains it and sends the messages there. Called outside `process()` they send immediately, as before. `terminate` stops the timer. Messages now reach the controller up to ~15 ms later, in the same order; controller code unchanged.
- Evidence (`tests/note_off`, now pass/fail): all 7 heap scenarios make 0 allocations and 0 frees inside `process()` and send 0 messages from it (before: up to 4835 allocations for 7 preset changes). The timer delivers exactly the messages the old code sent from the audio thread: preset changes 492 (old: 492 message allocations), knob sweep 7 (7), synced tempo change 3 (3). All earlier cases pass. Debug and Release build with 0 errors.
- Not covered: allocations inside a real host's `allocateMessage` now happen on the main thread (fine). Direct `malloc`/aligned `new` are not seen by the test (none found by grep in the audio path). A main-thread send can overtake older queued messages by up to one timer tick (both only update GUI state).

### Fix log: B-21 (2026-11-12)
- `CQBLimitedOscillator::doTriangle2`: width <= 0.5 uses the old formula unchanged (triangle -> rising saw). Width > 0.5 is the mirror image: the shape for `1 - width` played backwards (triangle -> falling saw), so the wave always stays within +-1 and width 0.3 and 0.7 are time-reversed copies. The step at the phase wrap (height `2 - 4w`, zero at 0.5) gets a 2-sample polyBLEP correction (down-step for <= 0.5, up-step for > 0.5).
- Evidence: new B-21 section in `tests/note_off` drives `CQBLimitedOscillator` alone (TRI, widths 0.1-0.9, f0 220 Hz and 1500 Hz, 65536-point FFT with f0 on an exact bin; "alias" = power off the harmonic bins / harmonic power).

| width | old min / DC / alias @220 Hz | new | old alias @1500 Hz | new |
|---|---|---|---|---|
| 0.1 | -1.000 / 0.133 / -24.5 dB | -0.988 / 0.133 / -40.7 dB | -16.1 dB | -31.8 dB |
| 0.3 | -1.000 / 0.150 / -29.5 dB | -0.984 / 0.150 / -45.7 dB | -21.1 dB | -36.4 dB |
| 0.5 | -1.000 / 0.000 / -67.8 dB | unchanged | -42.3 dB | unchanged |
| 0.7 | **-1.800** / -0.233 / -32.4 dB | -0.984 / 0.150 / -45.7 dB | -23.9 dB | -36.4 dB |
| 0.9 | **-2.600** / -0.514 / -28.7 dB | -0.988 / 0.133 / -40.7 dB | -20.2 dB | -31.8 dB |

- Not changed: the morph shape keeps a DC offset of 0.13-0.15 at widths away from 0.5 (it had that below 0.5 before; above 0.5 it was -0.23..-0.51); the filter/HP path removes some, a DC correction could be added. The polyBLEP uses the oscillator's own increment, so with heavy phase modulation from osc 2 it is approximate. The modulator (`WTOsc2`) uses the same class, so it gets the same fix when set to TRI. Patches with width > 0.5 on TRI (factory preset 6 modulator 0.69, preset 5 modulator 0.54) sound different now (no overshoot/DC jump).

### Fix log: B-01 + B-02 (2026-11-12)
Background found while fixing: the controller's two dispatch tables have different jobs. `onCtrlChange2` is engine sync: for the vertical switches it is `changeVertSwitch`, which sends the integer position to the processor (`PMSG_STEP_SWITCH`); the processor's `ctrlValues` (osc type, octave, interval, env sources, ...) are only ever set by that message. `onCtrlChange` is the GUI side (handlers that update `nvgView`, some unguarded). The old code skipped both, and the store, whenever the editor was closed.
- `DX10Controller::setParamNormalized` (`tag < GPARAMS`): always runs `onCtrlChange2` (engine sync). If the view is ready: GUI handlers + store as before. If not: stores the value and queues it in `pendingStateUpdates`, which `onNVGViewReady()` already replays into the GUI when the editor opens. Tags >= 173 (momentary step buttons, whose handler calls `performEdit`) are not stored or replayed, as before.
- `kPresetParam`: the preset is applied to the controller immediately whether the editor is open or not (stores the preset param itself too, which the old code never did), queues the GUI catch-up if closed, and `restartComponent` is null-guarded. `presetPending`/`checkPendingPreset` are no longer used.
- `BaseController::setComponentState` (editor closed): stores each value through `setParamNormalized` instead of only queueing it. Side effect: state values >= 173 are no longer replayed on editor open (the old replay would have called `performEdit` for step buttons).
- Evidence: new B-01/B-02 section in `tests/note_off` drives a headless `DX10Controller` (real `initialize`, counting peer, no editor). Old code: 4 of 4 checks fail (FILTER_ automation kept 0.93 instead of 0.37; OCT_ stored 0.0 and 0 switch messages; preset 2 -> 7 of 8 probed params wrong; 0 GUI updates queued). Fixed: all pass (95 GUI updates queued for the editor). All earlier cases pass. Debug and Release build with 0 errors.
- Not tested: the GUI replay itself when the editor opens (needs a host). The controller reads factory presets from `DX10Processor::programParams`, which the processor constructor fills (B-11); in the test a processor existed first.

### Fix log: B-06 (2026-11-12)
- `DXGenerator.cpp` (3 places: `noteOn` both modes, `processNoteOn`) now use `velocityGain(v)` = `clamp(v, 1, 127) / 127` (new inline in `DXGenerator.h`; change the curve there). Also removes a division by zero the old formula had for velocity 0.
- Evidence: new B-06 section in `tests/note_off` renders a held note at velocities 1-127 through the real `process()`. Old: levels 0 / 0.14 / 0.33 / 0.50 / 1.0 / 1.0 / 1.0 / 1.0 / 1.0 for 1/16/32/48/64/80/96/112/127. New: exactly v/127 (0.0079 ... 1.0), all distinct. All earlier cases pass. Debug and Release build with 0 errors.
- Effect on sound: everything below velocity 127 is quieter than before (velocity 64 was full level, now half). Existing patches played at medium velocity will sound softer; a curve (e.g. `sqrt` or `pow(x, 0.6)`) would keep more level at medium velocities.

### B-54  Font stash leaked on every editor close (~0.37 MB per open/close)  · HIGH · **FIXED 2026-11-12** (the measured cause of the "memory grows per window open" note)
`nanovg.c` `nvgDeleteInternal`: the call `fonsDeleteInternal(ctx->fs)` was commented out (`//RSEDITED`), so each close leaked the whole font stash (512x512 glyph atlas 262144 B, scratch buffer 96000 B, font tables 21120 + 8192 + 1536 + 1280 B = 0.372 MB). The probable reason it was disabled: `NVGView::attached` added the embedded font with `freeData = 1`, but `lucon_ttf` is a static array (`luconData.hpp`), so deleting the stash would `free()` static memory and crash. Side effect of the commented call: the bare `if (ctx->fs)` then governed the font-image loop below it. Also `fonsDeleteInternal` called `fons__tt_done(stash)` after `free(stash)` (use after free; harmless with stb_truetype, whose done is empty).

### Fix log: B-28 + B-29 + B-54 (2026-11-12)
- `NVGView::attached`: `nvgCreateFontMem(..., freeData = 0)` (static font data is not owned by fontstash).
- `nanovg.c`: `fonsDeleteInternal(ctx->fs)` restored, with braces, `ctx->fs = NULL`. `fontstash.h`: `fons__tt_done` before `free(stash)`.
- `NVGView::removed`: lock + make the view's GL context current around `nvgDeleteGL3`, unlock after (B-28).
- `NVGViewController` stores its owner; `~NVGViewController` calls the new `BaseController::onNVGViewControllerDestroyed`, which `DX10Controller` uses to null `nvgViewController` (B-29).
- New test `tests/editor_cycle` (build_and_run.bat [cycles]): opens the real editor (`createView("editor")`, attached to a real window, so the NanoVG/OpenGL view is created and draws for 400 ms) and closes it, N times; links the plugin objects and its .res; counts private memory, operator-new bytes, process-heap (malloc) bytes, handles, GDI/USER objects, live `NVGView`s, and whether the controller's pointer is dangling.
- Results, before -> after:
  - `NVGView` objects alive after N closes: 0 -> 0 (the view was always freed; an earlier idea that `remember()` in `setThreaded` leaked it was wrong).
  - operator-new heap growth per cycle: 0 -> 0. Handles / GDI / USER objects: flat -> flat.
  - malloc growth per cycle: **0.373 MB -> 0.002 MB** (heap diff showed the six font-stash blocks per cycle).
  - private memory over 200 cycles: 41.6 -> 144.8 MB (steady +0.38 MB/cycle after cycle 75) -> 42.6 -> 78.3 MB (+0.05 MB/cycle after 75, driver noise of +-20 MB).
  - controller pointer after close: freed object -> nullptr.
- The first open costs ~40 MB (GL context, driver, VSTGUI, fonts); that is one-time in this test. I did not reproduce "+30 MB per open" itself; in FL the per-open figure may include the host's own window and the first-open cost. Recheck in FL with Task Manager over ~10 open/close cycles.

### Fix log: B-11 + B-27 + B-52 (2026-11-12)
- B-11: the factory-preset assignments moved unchanged from the `DX10Processor` constructor into `DX10Processor::fillFactoryPresets()`; `DX10Processor::ensureFactoryPresets()` runs it once (function-local static, thread-safe). Called from the processor constructor and from the controller before it reads `programParams` (`setParamNormalized(kPresetParam)`, `checkPendingPreset`).
- B-27: `msgIdStr(char)` in `constants.h` returns a static 1-char C string; the four controller-side `setMessageID(&X_CHAR)` calls use it (`ctrlrReady`, `changeVertSwitch`, `changeLFORate`, the `PMSG_STEP_SWITCH2` reply in `BaseController::notify`).
- B-52: `processParameterChanges` ignores generator ids whose generator does not exist (55..99 with `GENCOUNT` 1). `recalculators[1000]` is now filled with a no-op (`recalcNone`) before the real entries; it was uninitialised, so an id in 100..999 without an entry called a garbage member-function pointer.
- Evidence: `tests/note_off` B-52 case sends ids 60, 99, 150 and 999 during a held note. Old code: access violation (crash). Fixed: no crash, note sounds and ends normally. `tests/editor_cycle` (no processor ever constructed) selects preset 2. Old: OSC_ 0.000, MOD_OSC_ 0.000 (empty table). Fixed: 0.900 / 0.600. B-27: controller section of `tests/note_off` checks every message ID is a 1-char string: 0 bad after the fix (not shown failing on the old code: the B-52 crash came first in that run). All other cases in both suites pass. Debug and Release build with 0 errors.

### B-55  Pan smoothing never ran → pan jump at note restart = pop  · MED · **FIXED 2026-11-12, confirmed by ear in FL Studio (2026-11-12, together with B-23)** (your report: dull pop every few seconds when tapping a note into its own decay, default patch, filter down)
`DXGenerator::singleVoiceProcessing` / `deadVoiceProcessing`: `prePan = pan;` was assigned before `panDiff = pan - prePan`, so the difference was always 0 and the slew limiter never ran; pan went to the output unsmoothed. Pan is applied after the filter. The default patch modulates pan (`PAN_LR_ENV` 0.16 from source 6, a per-voice LFO) and `processNoteOn` resets that LFO, so at every retrigger restart the pan jumped; with the filter down, the filter output is still non-zero at that moment, so the jump is a step in the signal. Its size depends on where the pan LFO was when the fade ended, which depends on your timing, hence "once every several seconds".

### Fix log: B-23 + B-55 (2026-11-12)
- B-23: `DX10Processor::notify` (`PMSG_STEP_SWITCH`) only writes `ctrlValues/controls` for `pid < NPARAMS`; `PMSG_LFO_RATE` bounds-checks `pid` before `recalculators[pid]`.
- B-55: pan is a one-pole glide (`prePan += panSmoothCoef * (pan - prePan)`, ~3 ms, coefficient set in `createVoice`) in both voice paths.
- Tool: `note_off_test.exe retrig [filter] [periodMs] [gateMs] [seconds]` (env `RETRIG_JITTER=1` for human-like timing/gate/velocity, `RETRIG_TRACE=1` for voice internals) renders the default patch with the processor and a real `DX10Controller` connected both ways (so the switch round trip happens as in a host), filter turned down, the same note tapped into its own release; reports per-note the largest second difference (click detector) and dumps the samples around the worst one.
- Evidence: before B-23 fix the probe went silent after ~1.3 s (gain ramp 0, cutoff 80 Hz); after, gain/cutoff stay at 0.75 / 385.8 Hz. Jittered 60 s run (194 notes, filter 0.3): before B-55 the 10 worst kinks were all at the restart (20.5 ms after note-on), up to 2.0x the median, and the samples show L/R snapping to equal values (pan jump to centre); after, no restart kink in the top 10, worst is 1.3x the median at the waveform peak (smooth). All `tests/note_off` and `tests/editor_cycle` checks pass.
- Still by design (not changed): a retrigger fades the old note to 0 over 21 ms, then restarts with oscillator phase reset and a fresh attack from 0. That dip is audible as a slight "re-articulation". A "retrigger from current level" mode (no fade, attack continues from the current envelope level, phase kept) would remove it; that is a sound-design choice.

### B-56  Modulation envelopes jump to 0 on a retrigger → "modulation drops out on retrig"  · MED · **FIXED 2026-11-12** (explains your note in `issues.txt`: "second voice drops out on retrig sometimes / -actually modulation drops out on retrig when env to high at retrig time")
`DXGenerator::processNoteOn` set `voiceEnvLfo[2].cenv` and `[3].cenv` (envelopes 2 and 3, the modulation sources) to 0 on every start, including the restart at the end of a retrigger fade. The amp envelope fades to 0 before that restart, but envelopes 2/3 jumped from their current level to 0 and re-attacked, so whatever they drive (mod depth = the "second voice", filter, pitch, width...) dropped out at the retrigger, by as much as the envelope's level at that moment.
Fix: `processNoteOn(..., bool retrig)`; the restart from `interpolateVoice` passes `true`, and then envelopes 2/3 keep their current level and attack from there (target reset to full as before). Fresh notes and stolen voices are unchanged.
Evidence (`tests/note_off` B-56, env-2 attack 0.6, decay hold, note re-struck at 1.0 s): env 2 before the re-strike 1.000; 50 ms after the restart old 0.763 (re-attacking from 0, same as a fresh note at 55 ms: 0.759), new 1.000. Retrigger pop probe unchanged (no restart kinks). Not changed: the per-voice LFOs still restart (phase and delay fade-in) on a retrigger; if modulation from an LFO still seems to drop out on retrig, that is the next candidate (a "keep LFO running on retrigger" option).

### Fix log: group 3, plumbing and cleanup (B-07, B-08, B-09, B-24, B-25, B-34, B-51) (2026-11-12)
- B-07: `BaseController::performEdit` returns `kResultFalse` for step-button tags (it fell off the end of a non-void function). The magic `173` is now `STEP_BUTTONS_FIRST` (= `STEP_CTRL_1`, `constants.h`) in both controllers.
- B-08: `BaseProcessor::setState` reads exactly the count of values the state holds (skips any beyond `GPARAMS`), checks every read, ignores non-finite values, and reads the bypass flag only if present; returns `kResultFalse` on a truncated state (`DX10Processor::setState` still applies what was read, then returns that result). `DX10Processor::setCurrentProgram` clamps the program to the 7 presets. `getState` writes `GPARAMS` as the count (identical bytes: `numParams` was already `GPARAMS`), so existing projects load exactly as before (round-trip test).
  Correction of the original note: a shorter state did **not** switch bypass on (`IBStreamer` stores 0 for a failed read); what it did was read on past the data and set every remaining parameter to 0.0, i.e. zero the rest of the patch.
  Evidence (`tests/note_off` B-08): round trip passes old and new. Old code: short state (10 params) -> ENV_1_DECAY 0.000 instead of preset 2's 0.930; program 50 in a state -> current program 50 (read past the 7 presets); truncated state -> reported success. New: 0.930, program 6, `kResultFalse`.
- B-09: the dead instance-id filter in `DX10Processor::notify` is removed (it compared a variable with itself). Not "fixed" on purpose: a connection point is per instance, and enabling it would drop messages from a controller that has not yet received `InstanceID`. Behaviour unchanged.
- B-24: `DXGenerator::RVoicesMap` is initialised to nullptr, allocated once in `init()` and freed in `~DXGenerator`.
- B-25: `doRecalc[FINE_ENV_SRC]` points at `recalcFineEnvSrc` (was `recalcFineEnv`); `interpolateVoice` ramps LFO 2/3 amplitude from their own value (was LFO 1's). Neither is audible (values are overwritten/unused). Not changed: `deadVoiceProcessing` still uses the last `gainerTemp` and no modulator mix during the 21 ms steal fade; `carPitchMultiplier` is still assigned and never read.
- B-34: `NVGView::valueChanged` delegates to `onResetButton` (right index, bounds/null checks).
- B-51 (SDK, `cbuttons.cpp`, logged in 05-sdk-mods): both functions return `kMouseEventNotHandled` on the fall-through path.
- All `tests/note_off` and `tests/editor_cycle` checks pass; Debug and Release build with 0 errors.

### Fix log: try/catch review + GUI hazards (B-30, B-31, B-32, B-33, B-38, B-53) (2026-11-12)
Your note: "lets see if we can do away with try catch everywhere, its probably my band aid for bad c++ clean up or timing". Inventory of the live sources: 15 `try { } catch (const std::exception&) { }` blocks in the `NVGView` GUI handlers (`onEnv`, `onEnvBipolar`, `onPanEnv`, `onEnvLev`, `onTuneSwitch`, `onOnOffBtn`, `onEnvSrc`, `onChangeOSCType`, `onChangeLFOOSCType`, `onChangeModType`, `onResetButton`, `onStepCtrlButton`, `onStepCtrlSwitch`, `onToggleButton`, `onRCButtonUp`), plus the registry read in the processor and controller constructors.
What they were hiding: almost nothing inside those handlers can throw a C++ exception. The real failures were null pointers and out-of-range indexing (`viewIndex[pid]` with no view, `envNameMap[pid]`/`envSrcNames[i]` null slots turned into `std::string`, `NVGParamDef[pid]` past the end, `resetSwitch[pid - WIDTH_RESET]` out of range), which crash and are never caught. The "timing" part was most likely B-29 (handlers reaching a freed `NVGView` through the dangling `nvgViewController` after the editor closed), fixed earlier today.
Changes:
- All 15 handler try/catch blocks removed. The failure cases are now impossible instead: `RandyTxt::reset` treats a null name as "" and gets a `(const char*, const char*)` overload; new bounds/null-safe lookups `paramName(pid)`, `envName(pid)`, `envSrcName(i)`, `viewAt(pid)` in `NVGView`; null checks in `initResetWidthAnim`, `onStepCtrlSwitch` (also clears pid 0's highlight, which `controlIdx != NULL` skipped), `onResetButton` (index range), `onRCButtonUp` (`find` instead of `.at`).
- One exception boundary kept on purpose, in `NVGView::drawOpenGL` (called by VSTGUI from the host's UI thread): any exception from the draw code (e.g. one of the ~76 map `.at()` lookups, B-38) skips that frame (`nvgCancelFrame`) instead of escaping into the host.
- `drawOpenGL`: returns if `vg` is null (B-30); animations for controls without a view are dropped instead of dereferencing null; the animation loop no longer skips the element after an erased one.
- `attached` / `removed` / `drawSelctedStepTarg`: null checks for the OCT_ view, `baseController`, `selectedStepTarg` (now initialised to nullptr) (B-30).
- SDK (logged in 05-sdk-mods): `CView` custom fields initialised (B-31); `std::stoi` -> `strtol` in `UIViewFactory` (B-32).
- Kept: the registry-read try/catch in `DX10Processor` / `BaseController` constructors. `GetStringValueFromHKLM` really throws when the key is missing, so that one is correct.
- Evidence: `tests/editor_cycle` now calls nine handlers with ids that have no name/view while the editor is open (e.g. `onStepCtrlSwitch(153)`, `onEnvLev(999)`, `onEnvSrc(.., 42)`, `onResetButton(5)`) and keeps drawing. Old code: access violation (0xC0000005) despite the try/catch. New: survives; all editor-cycle and note_off checks pass. Debug and Release build with 0 errors. Not covered: every handler/value combination; the per-frame `.at()` lookups were not rewritten (only contained).

### Fix log: B-20 + B-22 + B-47 (2026-11-12)
- B-20: `DXGenerator::recalcWidth` sets `widthPole/widthEdge` (main oscillator) instead of the modulator's `modWidthPole/modWidthEdge`. Factory presets are unchanged (all have main `WIDTH_ENV` 0.5 = zero amount, and the modulator's pole is set by `recalcModWidth`, which runs after `recalcWidth` on preset load). Changes: moving the main WIDTH below 0.5 no longer flips the modulator's width-env direction; your own patches with main width < 0.5 and a width env now push width downward (the documented intent).
- B-22: oscillator 1 clamps its note index to 0..127 before `midiFreqTable[]` (the modulator already clamps to 1..109).
- B-47: each modulation function clears the carrier field it does not use (`doPhaseMod` clears `m_dFoModLin`, `doFreqMod`/`doDetuneMod` clear `m_dPhaseMod`, `doNoMod` clears both).
- Evidence (`tests/note_off`): B-20 MOD_WIDTH 0.7 then WIDTH 0.3: old poles main +1 / mod -1, new -1 / +1. B-47 freq-mod for 0.5 s then none on a held note: old offset stays 77.755 Hz, new 0.000 Hz. B-22 MIDI 127 with octave 5 and interval 12: old 20.00 Hz (garbage read clamped to the frequency floor), new 12543.85 Hz (table top). MIDI 0 with octave 1 gives 20 Hz either way (the oscillator floors at 20 Hz). All other cases pass; Debug and Release build with 0 errors.

### B-53  Colliding 2-char message IDs, dead ID compares (extends B-27)  · LOW · **FIXED 2026-11-12** (`PMSG_STEP_CTRL_CHAR` is now an explicit `'c'` (same value); the unused `'s1'..'sB'` constants are removed. The two "dead compares" turned out to be inside comment blocks already.)
- (Same root as B-27, which was logged earlier; kept here for the extra findings below.) `constants.h:221-269` defines message IDs as single `char` constants and the processor passed `&PMSG_..._CHAR` to `setMessageID(const char*)`, which copies a C string: it read past the 1-byte constant until a 0 byte (undefined behaviour; the controller only looks at the first char, which is why it worked). Since B-10 the processor sends a proper 1-char string.
- `PMSG_STEP_CTRL_CHAR = 'sc'` and `PMSG_STEP_CTRL_N_CHAR = 's1'..'sB'` are multi-character literals truncated to one `char` (MSVC keeps the last char): `'sc'` -> `'c'`, `'s1'` -> `'1'` (same as `CMSG_LINK_TUNE_CHAR`), `'s2'` -> `'2'`, ... Only `PMSG_STEP_CTRL_CHAR` is sent (from `recalcStepCtrlN`), and `'c'` is unique, so no live collision found; the `_N_` constants are traps.
- Controller (`mdaBaseController.cpp:448,456`): `message->getMessageID == CMSG_ON_GET_SAMPLE_RATE` compares a member-function pointer, and `getMessageID() == "2"` compares pointers; both are always false (dead branches).

### B-52  Generator params 55-99 index past `generators[]`  · LOW→MED · **FIXED 2026-11-12** (worse than logged: unknown ids also called an uninitialised `recalculators[]` slot and crashed; see fix log)
`DX10Processor::processParameterChanges`: for `paramId < NPARAMS` (100) it uses `generators[paramId / PARTPARAMS]` with `PARTPARAMS` 55, but `GENCOUNT` is 1, so IDs 55-99 write to `generators[1]` (out of bounds). The controller only registers 0-54 (`PARTPARAMS * GENCOUNT`), so a host should never send them; reachable only with a bad/foreign param ID. Fix: bound by `PARTPARAMS * GENCOUNT`.

### Fix log: B-17 (2026-11-12)
Background (from the author): there is one preallocated voice per pitch (`RVoicesMap[128]`); `notes` is the list of pitches that `mainProcessing` renders every sample.
- `DX10Processor::doProcessing` calls `generators[0].detectSilence()` after every rendered chunk again (the call had been commented out).
- `DXGenerator::detectSilence` rewritten: a voice is dropped from `notes` only when it is not in a retrigger/steal fade (`rampingDown`) and both its amp-envelope target `env` and smoothed level `cenv` (env 1, which multiplies the voice output) are below 0.0001 (-80 dB). The old version tested only `cenv < 0.0003`, which would have dropped voices mid-fade (losing the restarted note and a B-18 pending release); checking `env` too keeps a note in its attack (cenv starts at 0.0003) and a held note. On removal it clears `killIt`/`releasePending`.
- `DXGenerator::noteOn` voice stealing: counts and steals only live voices (skips ones already fading out from a steal). Before, the fading voice was the quietest and was "stolen" again, so `POLYCOUNT` (32) was never enforced; this only became visible once voices leave the list.
- Effect on replays: a pitch whose voice finished now takes the fresh note-on path (`processNoteOn`) instead of the retrigger fade; for a finished voice that fade ended on the next sample anyway.
- Evidence: 5 new cases in `tests/note_off` (render-list size sampled every block). Old code: 60 released pitches -> 60 voices still rendered at 4.5 s (5 s render 2328 ms); 1 held + 20 released -> 21; replayed pitch -> still listed at 2.45 s; 40 held notes -> 40 voices. Fixed: 0 (1078 ms), 1, 0, 32; held note, slow attack (ENV_1_ATTACK 0.8) and replay all sound and end as expected. All earlier cases pass. Debug and Release build with 0 errors.
- Not changed: `std::deque` erase/push on the audio thread can free/allocate blocks (B-10). `RVoice::active` is never used; `RVoice::noteActive` is written but never read; `activevoices` is now counted again but nothing reads it (the old code used it to skip empty blocks; with an empty list `mainProcessing` is already cheap). The VST3 `silenceFlags` hint to the host comes from `checkSilence`, which scans the output and is independent of voices.
