# Parameters, messages, presets, state

Source of truth: `source/mdaDX10Processor.cpp`, `mdaBaseProcessor.cpp`, `mdaDX10Controller.cpp`, `mdaBaseController.cpp`. Line numbers are approximate and from Nov 2026.

## Parameter ID space

- `0 .. PARTPARAMS-1 (0..54)`: per-generator params (osc, width, tune, env sources, filter, pan, ...). Registered in `DX10Controller::initialize` by looping `GENCOUNT` x `PARTPARAMS` using `paramProto[][]` (`[0]` = constant, `[1]` = step count; 0 = continuous knob, >0 = `StringListParameter`). FILTER_ uses a `LogScaleParameter`.
- `110 .. ~170`: global env / LFO / reset / step-control params (`envProto[]`, `ENV_PARAMS`=52).
- `146 = MASTER_VOL` (2026-11-14): master output volume, see "Master volume" below. 144/145 are left free (vstgui.uidesc still has old control-tag names on them).
- Everything must be `< GPARAMS (1000)`; state and dispatch arrays are sized 1000.
- 4CC specials: bypass, preset (`'prst'`), mod wheel, pitch bend.
- Hard-coded `173` appears in several places (`beginEdit/performEdit/endEdit`, `DX10Controller::setParamNormalized`) as "last real param + 1". This is a magic number that must be kept in sync with `constants.h`.

## Flow 1: user turns a knob in the GUI

```
CControl (custom knob/button)  -> VST3Editor/Controller::valueChanged ... -> BaseController::performEdit(tag,val)
   (tag<173; linkTune mirrors TUNE_/FINE_ to MOD_ versions)  -> EditController::performEdit -> host
host -> Processor::process(data.inputParameterChanges) -> DX10Processor::processParameterChanges
   tag < NPARAMS(100):  floatParams[tag]=v; generators[tag/55].recalc2(tag%55)
   tag < GPARAMS:       floatParams[tag]=v; (this->*recalculators[tag])(v)   // env/LFO/step recalcs
```
Only the **last point** of each param queue per block is used; automation is not sample-accurate.

## Flow 2: processor -> controller messages (IMessage, not params)

The processor/controller keep a **side channel** of IMessages, with a 1-char message ID (`PMSG_*_CHAR`, `CMSG_*_CHAR`).
Threading (since B-10, 2026-11-12): the processor's `sendDoubleAttr/sendPidVal/sendIntAttr` called from inside `process()` only queue the message (lock-free ring); a 15 ms timer on the host main thread sends them. Called from any other thread (`initialize`, `setState`, ...) they send immediately. Messages:
- `InstanceID`: processor sends `this` pointer as int64 on `connect()`; controller stores it in `myInstanceId`. Both sides then stamp/compare `"id"` on messages (see bug B-09: processor compare is broken).
- `PMSG_STEP_SWITCH2` (processor -> controller): "set this vertical-switch/toggle from this value". Controller's `BaseController::notify` calls `doFromBase(pid,val)` (updates GUI) **and** replies with `PMSG_STEP_SWITCH` carrying the *string value as int* of the step param.
- `PMSG_STEP_SWITCH` (controller -> processor): `generators[0].ctrlValues[pid] / controls[pid] = int`. This is how step/vertical switches reach the DSP, **as a parallel path to normal parameters**.
- `PMSG_LFO_RATE` (controller -> processor): calls `recalculators[pid]`.
- `PMSG_LFO4/5/6_RATE`, `CMSG_LINK_TUNE`, `CMSG_GET_SAMPLE_RATE`: misc.
- Scope (2026-11-14): `CMSG_SCOPE_ENABLE_CHAR` 'e' (controller -> processor, int `on`; sent by `NVGView::attached/removed` via `BaseController::setScopeActive`) and `PMSG_SCOPE_CHAR` 'd' (processor -> controller, binary `data` = 1024 floats, sent from `BaseProcessor::onTimer`, never from `process()`). See docs/04-gui.md "Scope". Voice strip: `PMSG_VOICES_CHAR` 'f' (processor -> controller, binary `data` = 128 `voiceStateByte`s, sent by the same timer when they change, also only while an editor is open). Preset bar: `CMSG_SET_STATE_CHAR` 'h' (controller -> processor, binary `data` = a processor state in the `getState` layout, applied with `setState` in `notify`, main thread), `CMSG_GET_STATE_CHAR` 'i' (int `purpose`) answered by `PMSG_STATE_CHAR` 'j' (binary `data` = `getState`, same `purpose`). See docs/04-gui.md "Preset bar". Playheads: `PMSG_PLAYHEADS_CHAR` 'k' (binary `data` = 4 `PlayheadSlot`, timer, editor open; docs/04-gui.md "Playheads"). Output peak: `PMSG_PEAK_CHAR` 'g' (processor -> controller, float `peak` = largest |sample| of either channel since the last message, same timer, one 0 when the output goes silent, then nothing).

So step-type params have **two sources of truth**: the normalized param (`floatParams`) and the int sent by message (`ctrlValues`). They only agree if the message round-trip happens.

## Flow 3: presets (factory programs)

- Data: `static float DX10Processor::programParams[20][GPARAMS]`, filled in the **processor constructor** (`programParams[0][...]` = "preset 1" at `mdaDX10Processor.cpp:156`, `[1]` = "Prima Donna #0", ...); kNumPrograms = 7. (Earlier version of this doc wrongly said [0] was an all-zero init program.)
- Host selects program -> `kPresetParam` in param queue -> `DX10Processor::setCurrentProgramNormalized` -> `setCurrentProgram(i)`:
  copies `programParams[i]` into `floatParams`, recalcs all generator params + env/LFO recalculators, **sends `PMSG_STEP_SWITCH2` messages to the controller for each vertical switch and toggle**, re-inits ramped params.
- Controller side, `DX10Controller::setParamNormalized(kPresetParam)`:
  - view ready: sets *every* param on the controller from `DX10Processor::programParams[program][i]` (the controller reads the processor's **static array directly**), then `restartComponent(kParamValuesChanged)`.
  - view not ready: sets `presetPending=true` + `pendingPresetIndex`.
- `checkPendingPreset()` applies a pending preset, but see B-01: it is effectively never called.

## Flow 4: save/load state

- `BaseProcessor::getState`: writes `kMagicNumber`, current program, `numParams` (=1000), 1000 doubles of `floatParams`, bypass.
- `DX10Processor::setState`: calls `BaseProcessor::setState` then re-runs recalc2 for all part params, env/LFO recalculators, re-sends the step-switch/toggle messages, re-inits ramped params, `recalculate()`.
- Master volume (2026-11-14): after bypass, `getState` appends a tagged block `MASTER_VOL_STATE_TAG` ("MVOL") + one double. `setState` / `setComponentState` take the volume only from that block and use the default (0 dB) when it is missing; the value in the 1000-double array is ignored, because states saved before the parameter existed hold 0 in slot 146 (that would load as silence). Older builds stop reading after bypass, so they ignore the block.
- `BaseController::setComponentState`: reads the same stream. If `nvgViewReady`: applies each value now; else stores in `pendingStateUpdates[i]` and sets `hasPendingState`. `onNVGViewReady()` applies the map later.

- Controller state (2026-11-14): `DX10Controller::getState/setState` (the host's controller chunk) hold only the preset bar's name: tag "PBAR", version 1, factory index (-1 = none), edited flag, name (UTF-8), file path (UTF-8). Without the block (older projects) the name comes from the program number in the component state (`DX10Controller::setComponentState`).

- Polyphony (2026-11-15): `POLY_COUNT` (147), 1..`POLY_MAX` (64) voices, host parameter `POLYPHONY` with 63 steps (`polyFromNormalized`/`polyToNormalized` in constants.h), default 32. Global like MASTER_VOL: preset changes (processor and controller paths, preset bar) keep it, and the state stores it in a tagged block after the master volume (`POLY_STATE_TAG` "POLY" + double); states without it load at 32 (older states hold 0 in its array slot, which is ignored). `DX10Processor::recalcPolyCount` sets `DXGenerator::POLYCOUNT`. At the limit `DXGenerator::noteOn` steals a voice: released ones before held ones, then the quietest by max(env target, level) (the level alone made a note in its attack look quietest); the stolen voice fades out over ~21 ms. The original mono/portamento branches ran when POLYCOUNT was 1; they now need `DXGenerator::portamentoMode` (off), so 1 voice is a plain limit.

- State version (2026-11-15): `getState` ends with tagged blocks after bypass: MVOL, POLY, then `STATE_VERSION_TAG` "SVER" + double (`STATE_VERSION`, now 2). All readers (`BaseProcessor::setState`, `BaseController::setComponentState`, `presetTools::parseState`) take the blocks in any order. Parameters added after the first release would load as 0 from older states and preset files (their array slot was unused), so `newParamDefaults()` (constants.h) lists each with its default and the version it arrived in; `applyNewParamDefaults(version, set)` gives them their defaults when the state is older (also on truncated states, and for the factory preset rows). Add every new sound parameter there and bump STATE_VERSION when one arrives.
- Snap (2026-11-15): `SNAP_TIME` (148), the retrigger/steal fade (was a fixed 21 ms in `DXGenerator::saveTargValues`), 2..80 ms log (`snapMs` = 2 * 40^v), default 21 ms; part of the preset (version 2 parameter). `DX10Processor::recalcSnapTime` sets `DXGenerator::retrigFadeMs`; program changes reapply it. The fade is a linear ramp of the amp envelope to 0 over that time; a note released just before the replay also decays by its release, so it can end sooner. Host text in ms.

- Expression (2026-11-15): `EXPR_AMOUNT` (149, bipolar, centre = none), `EXPR_TARGET` (159: BRIGHT = filter + FM depth, FILTER, FM DEPTH, WIDTH), `EXPR_VEL` (158). Per voice and sample `e = amount` or, with VEL on, `amount * velocity/127`; offsets `e * depth` in knob units (`exprDepths`: BRIGHT 0.35 filter + 0.35 FM depth, FILTER 0.5, FM DEPTH 0.5, WIDTH 0.4 on both oscillators) are added to the filter cutoff before its clamp, to MOD GAIN inside the FM clamp (phase/freq/detune mod), and to the widths before their clamps. Part of the preset (version 2 parameters). Centre is bit-identical to no expression (`tests/note_off` "Expression").

- Drift (2026-11-15): `DRIFT_AMOUNT` (108), 0..1, default off. `DXGenerator::processNoteOn` (every note start and retrigger restart) picks per-voice offsets from a triangular -1..1 random (xorshift32 in the generator, no allocation): pitch factor 2^(amount * 15 cents * r / 1200) applied to both oscillator frequencies (the FM ratio stays), filter cutoff amount * 0.06 * r (knob units), pan amount * 0.3 * r. The generator's seed is fixed, so a render repeats exactly (every instance draws the same sequence). Part of the preset (version 2). Off is bit-identical (`tests/note_off` "Drift").

- HQ (2026-11-15): `HQ_MODE` (109), global like POLY_COUNT (`isGlobalParam`, tagged block `HQ_STATE_TAG`). `DX10Processor::applyEngineRate` runs the generator at twice the host rate (rates, envelope coefficients, voices rebuilt; sounding notes stop), and `doProcessing` renders two voice samples per output sample through `HqDecimator` (hqDecimator.h, 63-tap Blackman windowed sinc, cutoff 0.46 of the host rate, latency 15.5 host samples, not reported to the host). A switch is taken up at the start of the next chunk on the audio thread. Cost (`note_off_test perf` with `PERF_HQ=1`, 44.1 kHz): 61 -> 117 ns per voice-sample, 32 held voices 8.6 % -> 16.6 % of one core. Effect (`note_off_test alias <pitch>`): with the default patch the normal render has extra (aliased) energy above 5 kHz of -104 dB at C4, -75 dB at C6, -60 dB at C7, -50 dB at G7 relative to the total, which HQ removes; heavier FM patches alias more (not measured).

## The "view ready" gate (the most important cross-cutting pattern)

`BaseController::nvgViewReady`:
- set `true` in the **NVGView constructor** (`NVGView.h:490`), i.e. as soon as the view object exists, not when it is fully initialized;
- set `false` in `NVGView::removed()` (`NVGView.cpp:1140`);
- `NVGView::drawOpenGL()` calls `onNVGViewReady()` on **every frame** while ready (`NVGView.cpp:1394`) to flush `pendingStateUpdates`.
Consumers: `doFromBase`, `controlWasChanged`, `DX10Controller::setParamNormalized`, `setComponentState`, preset handling. Intent (from your notes): don't poke GUI objects before they exist. Side effect: **controller parameter values are not updated at all while the editor is closed** (B-02).

## Open questions for you
1. Is the intent that controller param values stay correct when the editor is closed (host automation, MIDI-learn, presets)? Right now they don't.
2. Why are step/vertical switch values sent via IMessage instead of being read from the param value in the processor? (Historic: strings->ints?)
3. (resolved) `programParams[0]` is a real preset, not an init program.

## Master volume (MASTER_VOL, 2026-11-14)

- Parameter 146, registered as `MasterVolParameter` (`mdaDX10Controller.cpp`): host name "MASTER VOLUME", unit dB, text from `paramDisplay::masterVolText` ("0.0", "-6.0", "-inf"); typed dB values are accepted (capped at +6).
- Taper: gain = 2 v^2 (`masterVolGain` in `constants.h`): 0 = silence, 0.7071 = 0 dB (default, so existing sounds are unchanged), 0.5 = -6 dB, 1 = +6 dB.
- Engine: `DX10Processor::doProcessing` multiplies both outputs by a one-pole smoothed gain (about 10 ms, coefficient set in `setActive`), target read from `floatParams[MASTER_VOL]` once per chunk.
- Global, not part of the presets: `setCurrentProgram`, the processor's `kPresetParam` paths and the controller's preset loops (`setParamNormalized(kPresetParam)`, `checkPendingPreset`) keep the current value. Every `programParams` row also holds the default, so a full-row copy never mutes.
- State: see Flow 4. GUI: knob `#masterVol` in the master row (docs/04-gui.md); readout via `onEnvBipolar` (label "VOLUME").
- Tests: `tests/note_off` section "Master volume" (level -6.02 dB at v 0.5, silence at 0, old state -> 0 dB, round trip, preset change, controller text).
