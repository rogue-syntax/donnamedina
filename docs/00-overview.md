# Donna Medina (RSSynth001) – Overview

VST3 instrument, vendor "Rogue Syntax". Built on Steinberg's **mda DX10 VST3 sample** (`mda*` files), heavily extended, with a custom NanoVG/OpenGL GUI on top of VSTGUI4. DSP classes derive from Will Pirkle book examples.

Status of this doc: pass 1 (skeleton) + pass 2 (params/state). Passes 3 (audio/DSP) and 4 (GUI/SDK mods) are not done yet. Items marked **[unverified]** come from reading only, not from running.

## What is actually live

`source/factory.cpp` registers **only**:
- Processor: `rs::DX10Processor` (`mdaDX10Processor.*`) – uid `3394EECF-198E4B66-80878031-E531ADD6`
- Controller: `rs::DX10Controller` (`mdaDX10Controller.*`) – uid `D304372E-393E4A29-B6D72C58-D268FD90`
- Name "Donna Medina", category `kInstrumentSynth`, distributable (separate component/controller).

Correction (verified after a challenge): my first version of this section was too broad.
- **Live, via `DXGenerator`:** `RVoice.h` (`DXGenerator` has `RVoice monoVoice` and methods like `processNoteOn(RVoice&)`, `retriggerVoice(RVoice&)`, `singleVoiceProcessing(RVoice&)`) and, through it, `WTOscillatorVoice.*`. These are core engine code, so your notes about RVoice/retrigger apply to the live plugin.
- **Compiled but not registered in the factory (dead at runtime):** `VSTSynthProcessor.*`, `VSTSynthController.*`, and what only they use: `Voice.*`, `RSSynth002Voice.*`. They are in the `.vcxproj` compile list and not excluded, so they build into the DLL but nothing in the factory instantiates them. `factory.cpp` has their includes commented out.
- Still unverified: whether `RSOGProc.h` / `RSOGView.h` (included by `NVGView.h` / `RVoice`-related code) are live.

## Class map (live)

```
AudioEffect (SDK)
 └ rs::BaseProcessor        mdaBaseProcessor.*   process(), state, bypass, message helpers
    └ rs::DX10Processor     mdaDX10Processor.*   note events, param recalc table, programs
        owns: DXGenerator generators[GENCOUNT=1]  (DXGenerator.*  – the real synth engine)
              Env EnvLfoCtrls[7], CLFO mainEnvLfo[7], ParamRampObj rampedParams[]

EditControllerEx1 (SDK)
 └ rs::BaseController       mdaBaseController.*  params, state restore, messaging, "view ready" gate
    └ rs::DX10Controller    mdaDX10Controller.*  param registration, onCtrlChange dispatch tables, createView
        createView -> RSView (RSView.*, a VST3Editor subclass, uidesc "vstgui.uidesc")
        createSubController("nvgViewController") -> NVGViewController -> NVGView (NanoVG GL view, ~2.9k lines)
```

Support: `constants.h` (all param IDs + sizes), `pluginobjects.cpp` (Pirkle-style DSP objects), `synthfunctions.h`, `ska.h` (hash map), `regReader.h` + `libbcrypt` (registration check), `nanovg*.c/h`, `GLShader.*`, `revent/` (event system, see revents notes).

## Key constants (`constants.h`)

| Name | Value | Meaning |
|---|---|---|
| PARTPARAMS | 55 | params per generator ("part") |
| GENCOUNT | 1 | generators (was 2 in older design) |
| NPARAMS | 100 | upper bound of per-generator param ID range |
| GPARAMS | 1000 | total param slots (state size, array sizes) |
| ENV_1_ATTACK | 110 | start of global env/LFO/step params |
| STEP_CTRL_TARG | 143 | |
| ENV_PARAMS | 52 | |
| _GEN_STEP_PARAM_COUNT_ | 23 | "vertical switch" params (see vertSwitchPids) |
| TOGGLE_COUNT | 7 | toggle buttons |
| EVENTBUFFER | 1024 | note event slots per block (3 ints per event) |
| kNumPrograms | 7 | factory presets (programParams is [20][GPARAMS]) |

Special (non-GPARAMS) IDs are 4CC values in `mdaBaseController.h`: `kBypassParam 'bpas'`, `kPresetParam 'prst'`, `kModWheelParam 'modw'`, pitch bend, `kLinkGen1Param 'lg1p'`.

## Registration / "bad reg" gate (traced pass 5)

Flow:
1. `DX10Processor::DX10Processor()` (`mdaDX10Processor.cpp:51-81`) holds the hard-coded secret `"4d01956a26c040343d6593d93bfe2e80"`.
2. `GetStringValueFromHKLM` (`regReader.h`) reads `Software\Rogue-Syntax\Registrations\DonnaMedina\RSRegKey` (a REG_SZ). **Despite its name it opens `HKEY_CURRENT_USER`, not HKLM** (the function name and my earlier text were wrong). Failure to read throws; the ctor catches and stores the message in `regErr1`.
3. The value is converted to UTF-8 and checked with `BCrypt::validatePassword(secret, storedHash)` (libbcrypt `bcrypt_checkpw`: hashes the secret using the salt/cost inside the stored hash, constant-time compares; a missing/garbage hash returns non-zero, i.e. false).
4. Result is stored in two places: `DX10Processor::regKeyLegit` (member, default false) and `constants::isRegLegit` (static, `constants.cpp:4`, default false).

What the flag blocks:
- **Audio:** only `DX10Processor::noteOn` (`:1396`): `if (regKeyLegit == true) { ...note on/off handling... }`. Both note-on and note-off events go through this function, so unregistered = every MIDI note is dropped = silence. Parameters, presets, state, editor all still work.
- **GUI:** `DX10Controller::createSubController` (`mdaDX10Controller.cpp:103-108`) copies `constants::isRegLegit` into `nvgView->badReg` once, when the editor is created; `drawOpenGL` then draws `drawBadReg()` on top of everything every frame (a black panel: "No registration detected! Please go on-line to rogue-syntax.com and register."). The panel covers only the middle of the window (237,90, 646x341); it does not disable controls under it. Other uses of `regKeyLegit`: a dead `NVGView::regKeyLegit = true` member and commented-out code in `BaseController` (the active controller is `DX10Controller`).
- Not blocked: nothing in `process()`, state, presets or the DSP checks the flag; there is no demo/timeout mode.

Notes (see B-45, B-46): the check runs only in the processor constructor (once per instance, a full bcrypt hash each time), so registering while the plugin is loaded needs a re-instantiate; the controller trusts the static, which is only correct if the processor was constructed first in the same process. If the synth is silent on a machine, check HKCU for the key for the **user the host runs as**.

## Dead code candidates (confirm before deleting)

- Compiled but not registered in the factory: `VSTSynthProcessor/Controller`, `Voice.*`, `RSSynth002Voice.*` (only referenced by the VSTSynth* pair).
- Backups/duplicates (RVoice.h itself is LIVE, see above): `RVoice_bkp.h`, `WTOscillatorVoice_bkp.cpp`, `DXGenerator (2).cpp`, `DXGenerator_avx.cpp`, `constants.bkp.h`, `uidescription test.h`. `RSOGProc/View`: unverified.
- Commented-out old implementations inside `mdaDX10Processor.cpp` (doProcessing ~1090, processEvents ~1201, noteOn ~1357).
- Folders: `oldx/`, `removedFromSrc/`, `source_bkp/`, `vstgui.uidesc.bkp`.
- `Source.cpp` is an empty file but is in the compile list.
- The project also compiles `vst2wrapper.sdk.cpp` (a VST2 wrapper) – check whether intended.

## Docs
- `01-params-and-state.md` — params, messages, presets, state (pass 2)
- `02-voices-and-dsp.md` — voice engine, note lifecycle, signal chain (pass 3)
- `03-modulation.md` — sources, envelopes, LFOs, smoothing (pass 3)
- `BUGS.md` — ledger with verification log
- `06-note-event-trace.md` — host event to voice trace, ghost-note timing model (pass 5)
- `TRIAGE.md` — all findings grouped as "look into / possibly fix" (no fixes scheduled)

## Next passes
- (done) 02 voices & DSP, 03 modulation
- 04 GUI (NVGView, custom controls, revent) — started, see 04-gui.md
- 05 SDK mods (pass 4 reconstruction added to 05-sdk-mods.md)
