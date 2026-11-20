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
- **Dead, no longer compiled (since 2026-11-12):** `VSTSynthProcessor.*`, `VSTSynthController.*`, `Voice.*`, `RSSynth002Voice.*`, `Source.cpp`, and the VST2 wrapper. The files are still in `source/` (not deleted).
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

Support: `constants.h` (all param IDs + sizes), `pluginobjects.cpp` (Pirkle-style DSP objects), `synthfunctions.h`, `ska.h` (hash map), `license.*` + `external/monocypher` (licensing v1, see docs/07-licensing.md), `nanovg*.c/h`, `GLShader.*`, `revent/` (event system, see revents notes).

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

## Licensing gate (licensing v1, 2026-11-19)

See `docs/07-licensing.md` (spec: `docs/store/licensing-v1.md`, the store's). In short:
- **Token:** an Ed25519-signed token (`RS1.` prefix) stored in `HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina`, value `License`, written by the installer (from `license.txt`) or the editor's **Paste license** buttons.
- **Processor:** `DX10Processor::refreshLicense` (constructor, and on `CMSG_LICENSE_CHANGED_CHAR`) sets `regKeyLegit`. Only `noteOn` checks it (`licensedNow()`: also the process-wide `license::sessionLicensed`, so a paste in one instance unlocks all), so unlicensed means silence. An unlicensed processor also watches the registry key (another process's paste).
- **Controller:** `BaseController::refreshLicense` (`initialize`, every editor open) sets `licensed`. `NVGView` draws the "Not registered" screen (Buy, Paste license) while it is false.
- **Update check** (2026-11-20, `source/updateCheck.*`): once a day, in the background, when the plugin opens; "Update available" badge and About line. Never affects licensing.
- **Removed:** the old bcrypt check, its hard-coded password and `constants::isRegLegit`. The old `RSRegKey` value is ignored.

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
