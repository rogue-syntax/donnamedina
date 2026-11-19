# Voices and DSP (pass 3)

Files: `source/DXGenerator.{h,cpp}` (engine), `RVoice.h` (per-voice state), `QBLimitedOscillator.*` + `Oscillator.h` (oscillators), `LFO.h` + `CLFOscillator.h` (per-voice LFO *and* envelope), `SEMFilter.h` (filter), `ParamRampObj.*` (smoothing), `lookuptables.h`. Everything here was read, not run. Line numbers approximate.

## Shape of the engine

- `DX10Processor::generators[GENCOUNT=1]` is a single `DXGenerator`. (The "generator" abstraction is left over from a 2-generator design; `paramStartIdx = PARTPARAMS*genIdx`.)
- `DXGenerator::RVoicesMap` is a plain `RVoice[128]` (`new` in `init()`), despite the name "Map". **In the default mode (`cutNote = true`, `DXGenerator.h:389`) the voice index is the MIDI note number**, so a pitch can only be sounding once; a repeated pitch re-uses (retriggers) its voice. `noteID` is not used by the engine in this mode.
- `std::deque<int32> notes` (`DXGenerator.h:555`) is the list of "active" voices (holds note numbers). `POLYCOUNT = 32` is the intended polyphony cap (`:550`).
- Audio is rendered **one sample at a time** (`DX10Processor::doProcessing` calls `generators[0].mainProcessing()` per sample), summing `o1` / `o2` (L/R) over every voice in `notes`.
- **Since 2026-11-19 (branch `simd-voices`, step 1 of the SIMD voice work):** `doProcessing` calls `DXGenerator::renderBlock(n)` instead, up to `kRenderMax = 256` engine samples at a time (HQ: 2 per output sample), into `renderL` / `renderR`; the FX strip and master gain then run per sample over that buffer. `renderBlock` loops **voice by voice**, each over the whole block: it precomputes the ramped-parameter levels per sample (`rampRows`, copied back into `rampedParams` per sample only when a ramp moves), calls `updateLfoUsage` once per block, keeps `sampleClock` per sample (a fade ending mid-block stamps the restart), and sums voices in `notes` order, so the output is **bit-identical** to the per-sample loop (`golden` and `GOLDEN_HQ=1 golden`: 0 difference). `mainProcessing` is kept but no longer called. Only exception, in principle: with DRIFT on, two fades ending in the same block draw their random drift in voice order rather than time order (each voice still gets a random value).
- **Step 2 (voice bank):** each voice writes its own output row (`voiceRowL/R[notes index]`), summed in `notes` order at the end. Voices that pass `bankEligible` (not fading; carrier and modulator SINE/SAW1/TRI/SQUARE; mod type 0..3; every modulation source 0..6) render in groups of 4 through `renderBankGroup` (`DXGeneratorBank.cpp`, state in `VoiceBank.h`): gather the hot state into column arrays, run the block with the lane loop inside the sample loop, scatter back. The per-lane code is a line-by-line port of `singleVoiceProcessing` and what it calls (LFO update/oscillate, the oscillators, the mod functions, the filter), so **a change to the voice path must be made in both places**. The rest (fades, SAW2/SAW3/noise/RSINE) still goes through `singleVoiceProcessing`. `useVoiceBank = false` (test env `NO_BANK`) switches the bank off.
  - Step 3a: the bank is one component per engine class (`voicebank::Voice4` holding `Envelope4`, `Lfo4`, `Osc4`, `Filter4`, `Sources4`), each with gather / scatter / step mirroring the class. Step 3b-0: inside a sample, each stage runs for all 4 lanes before the next (a lane loop per stage, so a stage can become one SIMD operation); spare lanes of a partial group hold a copy of lane 0; a leftover group of 1-2 voices goes through the scalar path (cheaper). The compiler already vectorises part of the lane loops: 32 voices 32 ns per voice-sample vs 49-50 without the bank, 8 voices 38 vs 55 (5.8% -> ~4.4% of a core at 32 voices). From 3b-0 on the output is no longer bit-identical in every case: with DRIFT on (pitch factor not exactly 1), the vectorised code rounds differently (fused multiply-add), 137-140 dB below the signal, max 6e-7 per sample (`GOLDEN_KEYSYNC`); `golden`, `GOLDEN_HQ` and the `GOLDEN_OSC` combinations are still 0 difference. Acceptance from here: about 110 dB below the signal or better (as for the AVX2 build flag).
  - Step 3b-1: hand-written AVX2 for the carrier and modulator tuning (integer steps, MIDI table gather, tune, width), `Osc4::updateAll` and the filter (`Filter4::setGAll` / `processAll`, coefficient table gather). Measured share before (cycle counters per stage): tuning 42 %, filter 16 %, carrier oscillator + gain 12 %, envelopes 10 %. Now 32 voices 22-23 ns per voice-sample vs 49 without the bank (3.2 % of a core; HQ 6.2 %), 8 voices 29 vs 56. All golden variants back to 0 difference (written out by hand, no fused multiply-add, so the same roundings as the scalar code). Still scalar per lane: LFOs, the oscillator waveforms (`Osc4::step`, BLEP), mod type, gain, pan.
  - Verified bit-identical (0 difference, steps 2 and 3a) against the per-sample engine: `golden`, `GOLDEN_HQ=1`, `GOLDEN_KEYSYNC=1` (key sync + drift: fading voices on the scalar path mixed with bank voices), and `GOLDEN_OSC=c,m,t` for every carrier x modulator x mod type the bank takes, plus fallback waveforms. A/B recipe: `set GOLDEN_FILE=...`, `NO_BANK=1 note_off_test golden save`, then `note_off_test golden`.

## Per-sample flow (`mainProcessing`, `:708`)

```
for each ramped param:  rampedParams[i].processLevel()          // gain, width, filter, pan... smoothing
for each voice n in notes:
    if voice.rampingDown:  interpolateVoice(v)                  // retrigger/kill fade, see below
    else:                  singleVoiceProcessing(v)
    sum o, o1 (left), o2 (right)
```

`singleVoiceProcessing` (`:785`) per voice, per sample:
1. Update envelopes 1-3: `env *= cdec; cenv += catt * (env - cenv)` (one-pole smoothed exponential decay). Env 1 is the **amp** envelope.
2. Update LFOs 4-6 (per-voice instances; level/rate/phase read live from the global `EnvLfoCtrls[4..6]`).
3. Run the **modulation type** function: `modChoice[ctrlValues[MOD_TYPE_]]` = 0 none, 1 phase mod, 2 freq mod, 3 detune (see 03).
4. Compute oscillator-1 pitch: `newNote = note + newInt + (newOct-3)*12` (`newOct` clamped 1..5), `freq = midiFreqTable[newNote] + midiFreqTable[newNote]*2*tuneC` (`tuneC` = tune + env*tune_env + fine). **No clamp on `newNote`** (see B-22). Oscillator 2 (modulator) does the same but clamps note to 1..109.
5. Width: `triCutoff = ramped WIDTH + WIDTH_ENV*widthPole*env`, clamped 0.1..0.9.
6. `osc1pure = WTOsc.doOscillate()`; `o = gain * ampEnv.cenv * (osc1pure + osc2out * MOD_OSC_GAIN)`, where `gain = clamp(ramped GAIN + GAIN_ENV*env, silenceCutoff, 1)`.
7. Filter: cutoff = `m_pFilterLogParam->toPlain(clamp(ramped FILTER + FILTER_ENV*env, 0.01, 1))` (log-scale mapping evaluated **per voice per sample**), `m_Filter1.update()`, `doFilter2(o)` (in place; `CSEMFilter`).
8. Pan (equal-ish linear): `o1 = (pan+1)/2*o`, `o2 = (1-pan)/2*o`. The pan "smoothing" block is a no-op (`prePan = pan` immediately before computing the difference).

Oscillators: `CQBLimitedOscillator` (quasi-bandlimited). `m_uWaveform` enum is `SINE,SAW1,TRI,SQUARE,SAW2,SAW3,NOISE,PNOISE,RSINE` (0..8); the GUI's OSC_/MOD_OSC_ lists have 4 steps (sine, saw, tri, square). **TRI (`doTriangle2`) and SQUARE (`doSquareO`) use the width (`triCutoff`)**: TRI as a shape morph (since the B-21 fix: width 0.1 rising saw <- 0.5 triangle -> 0.9 falling saw, always within +-1, polyBLEP on the edge), SQUARE as pulse width via sum-of-saws (`m_dPulseWidth = triCutoff`, zero-mean, levels -1 / +(1-p)/p, so extreme widths are quieter). SINE and SAW ignore it. (Corrected in pass 5: earlier text said square ignored width.)

## Note lifecycle

`DX10Processor::noteOn(note, vel, noteID)` (gated by `regKeyLegit`) -> `DXGenerator::noteOn` (`:322`), `cutNote == true` branch:

1. Set `RVoicesMap[note]` fields (note, vel, velocity (integer-division curve, see B-06), `noteActive = true`).
2. Is the note already in `notes`? (`pushNote`)
   - **No (new pitch):** if `notes.size() >= POLYCOUNT`: pick the voice with the lowest amp `cenv`, `retriggerVoice(that, killIt=true)` (fade it out over ~21 ms), then `notes.push_back(note)` and `processNoteOn` (start immediately). Else just push and `processNoteOn`.
   - **Yes (same pitch already in the list, even if long finished):** depends on OSC KEY SYNC (`KEY_SYNC`, `DXGenerator::keySync`, 2026-11-17):
     - **off (default), soft retrigger:** `processNoteOn(v, ..., retrig=true, soft=true)` at once: envelopes 1-3 re-attack from their current level (`env` = target, `cenv` kept), the oscillators keep their phase, the per-voice LFOs, the filter and the drift offsets keep running. No delay, nothing jumps. (The attack's one-pole curve starts at full speed, so a fast attack bends the waveform's slope at the re-attack, as on an analog envelope.)
     - **on (or the voice is already fading):** `retriggerVoice(v, killIt=false)`: `rampingDown = true`, `fadePos = 1`, `fadeStep` = 1 / (Snap time in samples). A voice already fading keeps its fade position.
3. While `rampingDown`: `interpolateVoice` renders the voice through `singleVoiceProcessing` (B-64) and multiplies its output (`o`, `o1`, `o2`, after the filter) by an S-curve of `fadePos` (`s*s*(3-2s)`), then steps `fadePos -= fadeStep`. At 0: `rampingDown = false`; if `killIt == false`, `processNoteOn` restarts the note from the stored `note`/`vel` (resets oscillators, LFOs and the filter) and applies a pending release (B-18); if `killIt == true` (stolen), env 1 is set to 0 and the filter reset, so the voice is silent until `detectSilence` drops it. Until 2026-11-17 the fade was a linear ramp of the amp envelope, which sits before the filter, so a resonant filter was still ringing when the restart changed its settings (B-65).

`processNoteOn` (`:431`) copies waveform/oct/mod settings, re-inits all envelope fields (`env1 = velocity`, `cenv1 = 0.0003`; envs 2/3 `env=1, cenv=0`; `catt/cdec` copied from the global `EnvLfoCtrls[n]`), resets LFO phase/delay, caches the "step" control values (`ctrlValues[...]`) into the voice, sets the filter cutoff and clears the filter's memory (`m_Filter1.reset()`; the voice is silent at that point). With `retrig` envs 2/3 keep `cenv` (B-56); with `soft` (key sync off) it skips the oscillator, LFO, drift and filter resets and env 1's `cenv`.

`noteOff(note)` (`:563`, `cutNote` branch): for envs 1..3 either switch the decay to the release rate (`cdec = crel`, when `decayReleaseToggle < 0.5`) or leave decay as is; then `env = cenv; catt = 1` (release starts from the current smoothed level) and `noteActive = false`. **`noteActive` is written in three places and never read** (grep), so it has no effect.

Voices are removed from `notes` only by `DXGenerator::detectSilence`. Until 2026-11-12 it was never called (B-17); now `doProcessing` calls it after every chunk, and it drops a voice once it is not fading (`rampingDown`) and its env 1 `env` and `cenv` are both below -80 dB. Voice stealing (`POLYCOUNT` 32) counts only live voices.

## Things that are shared vs per voice
- Global (one copy): `controls[GPARAMS]` (float, normalised/recalculated control values), `ctrlValues[NPARAMS]` (int step values, set by `PMSG_STEP_SWITCH` messages), `rampedParams[]`, `EnvLfoCtrls[7]` (settings object, `Env` = `CLFO` with extra fields).
- Per voice (`RVoice`): 2 oscillators, SEM filter, 7 `CLFO` (index 0 = constant 0 "off", 1-3 envelopes, 4-6 LFOs), `envBank[7]` pointers to each `cenv`.
- Envelope attack/decay/release of a voice are **copied at note-on**, so turning those knobs affects only new notes. LFO rate/level are read every sample.

## Memory/cost notes
- `RVoicesMap` is allocated once in `DXGenerator::init()` and freed in the destructor (B-24, fixed 2026-11-12).
- Before the B-17 fix voices never left `notes`, so per-sample cost grew with the number of *distinct pitches played* (up to 128 voices, each doing two oscillators, filter, 3 LFOs, a log-scale lookup). Measured in `tests/note_off`: 5 s render after 60 pitches took 2328 ms before, 1078 ms after.
- Voice limit: `POLYCOUNT = 32` (quietest live voice is stolen beyond that; enforced since B-17). Cost is linear in voices, so raising it is a one-line change.

## Performance (2026-11-12)
Tools: `tests\note_off\bin\note_off_test.exe perf [voices...]` (held voices of the default patch, wall-clock; `PERF_PROFILE=1` adds a 1 ms sampling profiler, `PERF_SECONDS=n` the duration) and `note_off_test.exe golden [save]` (renders all 7 presets through processor + real controller and compares sample by sample with a saved reference; run `golden save` on the old code first). Rebuild the test (`build_and_run.bat <args>`) after rebuilding the plugin: it links the plugin's object files.

| Step | 32 voices, ns per voice-sample | Sound vs before |
|---|---|---|
| baseline (after the bug fixes) | ~150-190 (~27% of one core) | - |
| inline `rsMax/rsMin` instead of CRT `fmin/fmax`; FTZ+DAZ in `process()` | ~126 | bit-exact |
| skip per-voice LFOs no destination reads (`updateLfoUsage`) | | bit-exact (see note) |
| filter coefficient from a 4096-point table `g(x) = tan(pi * toPlain(x) / fs)` instead of `powf` + `tan` per sample | ~52 (~7.5% of one core) | worst preset 115.8 dB below the signal (max sample diff 2.7e-6) |

So ~3x faster with the sound unchanged for practical purposes. Profile before: `tan` + filter update ~15%, `powf` (cutoff log map) ~11%, LFOs ~15%, CRT min/max ~11%. After: voice code 71%, oscillators ~18%, nothing else above 1%.
Tried and rejected: updating the filter at control rate (every 16/8/4 samples with a linear glide) gave the same speed but differed 28-39 dB below the signal (cutoff lags its modulation); the table is per sample and exact enough.
Note on the LFO skip: an LFO that no destination reads is not advanced. If you route a destination to that LFO while a note is already sounding, the LFO continues from where it stopped being computed (phase and delay fade-in), not from where it would have been. Fresh notes reset the LFO as before.
Not done (bigger refactor, diminishing returns): SIMD across voices (needs struct-of-arrays voices and no per-voice function-pointer dispatch), float-only voice maths, control-rate LFOs.
Benchmark caveat: measured with the default patch (one LFO routed). Patches that route all three LFOs gain less from the LFO skip.

## Filter and oscillator helpers (pass 5, read)
- `CSEMFilter` (`SEMFilter.h`): a trapezoidal (TPT) state-variable filter. `update()` per voice per sample: `g = tan(pi*fc/sr)`, `R = 1/(2Q)`, then `doFilter2` does the HP->BP->LP chain and returns **LP**. Cutoff range comes from the log map 80-18000 Hz (`DXGenerator.cpp:43-47`); Q from `cookVSTGUIVariable(MIN_FILTER_Q=1.0, MAX_FILTER_Q=2.5, ...)` then `setQControl` maps 1..10 -> Q 0.5..25, so the knob only reaches Q ~= 0.5..4.6 (B-41). No clamp of fc to Nyquist (B-42). A second "Butterworth" path (`updateButterworth/doButterworth`) is unused; its `b3` default initializer reads uninitialised `K`/`norm` (harmless) and `b2` is never set.
- Oscillator helpers (`QBLimitedOscillator.h`): saw = trivial saw + BLEP (`doBLEP_N`; 8-point Blackman-Harris table when fo <= sr/8, else 2-point); `dInc == 0` is patched to 0.003 inside `doBLEP_N` with the comment that zero increment "was crashing the plugin". Square = difference of two saws offset by the pulse width. These are textbook Pirkle routines; no problem found beyond what is logged.
- No denormal protection anywhere (B-40).

## LFO / envelope class and BLEP (pass 5, read)
- `CLFOscillator` (base for oscillators *and* LFOs) holds the shared phase/frequency fields (float), pitch-mod inputs and the `update()` that turns `m_dOscFo * ratio` (+ `m_dFoModLin`) into `m_dInc`; an LFO "delay" is implemented by forcing `m_dFo = 0` until `m_dDelayCounter` reaches 1 (the LFO phase stays frozen during the delay).
- `CLFO` adds `doSine/doSaw/doTri/doSqu` (`LFO.h`), plus the **envelope** machinery in the same object: `doEnvOp()`: `e = env; env = e*cdec; cenv += catt*(e - cenv)` (matches 03). `env` and `cenv` are floats that are never floored, which supports B-40. LFO output is `dOut * m_dAmplitude * m_dAmpMod`.
- LFO waveforms 0..3 = sine (parabolic approximation), saw (inverted ramp, see B-43), tri, square (`m_dModulo > m_dPulseWidth/100`, default 50%). Wrap handling is `checkWrapModulo` (subtract 1 once).
- BLEP tables (`dBLEPTable*`, 4096 points) are pre-baked constants in `synthfunctions.h`, not generated. I checked the index maths in `doBLEP_N`: left side index <= table centre, right side guarded (`+1 >= len` wraps to 0), so no out-of-range read for modulo in [0,1).
- Out of scope per you: mono/portamento (`POLYCOUNT == 1`) and `cutNote == false`.

## Not read yet
`CLFOscillator.h` internals, the BLEP table generation, mono/porto mode (`POLYCOUNT == 1`), `cutNote == false` mode (looks unfinished: `noteOff` does nothing there), `DXGenerator.h` members not touched above.
