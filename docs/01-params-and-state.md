# Parameters, messages, presets, state

Source of truth: `source/mdaDX10Processor.cpp`, `mdaBaseProcessor.cpp`, `mdaDX10Controller.cpp`, `mdaBaseController.cpp`. Line numbers are approximate and from Nov 2026.

## Parameter ID space

- `0 .. PARTPARAMS-1 (0..54)`: per-generator params (osc, width, tune, env sources, filter, pan, ...). Registered in `DX10Controller::initialize` by looping `GENCOUNT` x `PARTPARAMS` using `paramProto[][]` (`[0]` = constant, `[1]` = step count; 0 = continuous knob, >0 = `StringListParameter`). FILTER_ uses a `LogScaleParameter`.
- `110 .. ~170`: global env / LFO / reset / step-control params (`envProto[]`, `ENV_PARAMS`=52).
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

The processor/controller keep a **side channel** of IMessages, with a 1-char message ID (`PMSG_*_CHAR`, `CMSG_*_CHAR`):
- `InstanceID`: processor sends `this` pointer as int64 on `connect()`; controller stores it in `myInstanceId`. Both sides then stamp/compare `"id"` on messages (see bug B-09: processor compare is broken).
- `PMSG_STEP_SWITCH2` (processor -> controller): "set this vertical-switch/toggle from this value". Controller's `BaseController::notify` calls `doFromBase(pid,val)` (updates GUI) **and** replies with `PMSG_STEP_SWITCH` carrying the *string value as int* of the step param.
- `PMSG_STEP_SWITCH` (controller -> processor): `generators[0].ctrlValues[pid] / controls[pid] = int`. This is how step/vertical switches reach the DSP, **as a parallel path to normal parameters**.
- `PMSG_LFO_RATE` (controller -> processor): calls `recalculators[pid]`.
- `PMSG_LFO4/5/6_RATE`, `CMSG_LINK_TUNE`, `CMSG_GET_SAMPLE_RATE`: misc.

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
- `BaseController::setComponentState`: reads the same stream. If `nvgViewReady`: applies each value now; else stores in `pendingStateUpdates[i]` and sets `hasPendingState`. `onNVGViewReady()` applies the map later.

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
