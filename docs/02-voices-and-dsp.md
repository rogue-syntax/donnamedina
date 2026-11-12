# Voices and DSP (pass 3)

Files: `source/DXGenerator.{h,cpp}` (engine), `RVoice.h` (per-voice state), `QBLimitedOscillator.*` + `Oscillator.h` (oscillators), `LFO.h` + `CLFOscillator.h` (per-voice LFO *and* envelope), `SEMFilter.h` (filter), `ParamRampObj.*` (smoothing), `lookuptables.h`. Everything here was read, not run. Line numbers approximate.

## Shape of the engine

- `DX10Processor::generators[GENCOUNT=1]` is a single `DXGenerator`. (The "generator" abstraction is left over from a 2-generator design; `paramStartIdx = PARTPARAMS*genIdx`.)
- `DXGenerator::RVoicesMap` is a plain `RVoice[128]` (`new` in `init()`), despite the name "Map". **In the default mode (`cutNote = true`, `DXGenerator.h:389`) the voice index is the MIDI note number**, so a pitch can only be sounding once; a repeated pitch re-uses (retriggers) its voice. `noteID` is not used by the engine in this mode.
- `std::deque<int32> notes` (`DXGenerator.h:555`) is the list of "active" voices (holds note numbers). `POLYCOUNT = 32` is the intended polyphony cap (`:550`).
- Audio is rendered **one sample at a time** (`DX10Processor::doProcessing` calls `generators[0].mainProcessing()` per sample), summing `o1` / `o2` (L/R) over every voice in `notes`.

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

Oscillators: `CQBLimitedOscillator` (quasi-bandlimited). `m_uWaveform` enum is `SINE,SAW1,TRI,SQUARE,SAW2,SAW3,NOISE,PNOISE,RSINE` (0..8); the GUI's OSC_/MOD_OSC_ lists have 4 steps (sine, saw, tri, square). **TRI (`doTriangle2`) and SQUARE (`doSquareO`) use the width (`triCutoff`)**: TRI as a shape morph (see B-21), SQUARE as pulse width via sum-of-saws (`m_dPulseWidth = triCutoff`, zero-mean, levels -1 / +(1-p)/p, so extreme widths are quieter). SINE and SAW ignore it. (Corrected in pass 5: earlier text said square ignored width.)

## Note lifecycle

`DX10Processor::noteOn(note, vel, noteID)` (gated by `regKeyLegit`) -> `DXGenerator::noteOn` (`:322`), `cutNote == true` branch:

1. Set `RVoicesMap[note]` fields (note, vel, velocity (integer-division curve, see B-06), `noteActive = true`).
2. Is the note already in `notes`? (`pushNote`)
   - **No (new pitch):** if `notes.size() >= POLYCOUNT`: pick the voice with the lowest amp `cenv`, `retriggerVoice(that, killIt=true)` (fade it out over ~21 ms), then `notes.push_back(note)` and `processNoteOn` (start immediately). Else just push and `processNoteOn`.
   - **Yes (same pitch already in the list, even if long finished):** `retriggerVoice(v, killIt=false)`: `rampingDown = true`, compute a linear ramp-down step for the amp env (`env = cenv; envInc = -(1000*env)/21/sampleRate`, i.e. reaches 0 in ~21 ms).
3. While `rampingDown`: `interpolateVoice` steps `env += envInc`; **when `env < 0.0003`** it sets `rampingDown=false` and, if `killIt == false`, calls `processNoteOn` to (re)start the note from the stored `note`/`vel`. If `killIt == true` the voice just fades (`deadVoiceProcessing`) and then stays silent.

`processNoteOn` (`:431`) copies waveform/oct/mod settings, re-inits all envelope fields (`env1 = velocity`, `cenv1 = 0.0003`; envs 2/3 `env=1, cenv=0`; `catt/cdec` copied from the global `EnvLfoCtrls[n]`), resets LFO phase/delay, caches the "step" control values (`ctrlValues[...]`) into the voice, sets the filter cutoff.

`noteOff(note)` (`:563`, `cutNote` branch): for envs 1..3 either switch the decay to the release rate (`cdec = crel`, when `decayReleaseToggle < 0.5`) or leave decay as is; then `env = cenv; catt = 1` (release starts from the current smoothed level) and `noteActive = false`. **`noteActive` is written in three places and never read** (grep), so it has no effect.

Voices are removed from `notes` only by `DXGenerator::detectSilence`. Until 2026-11-12 it was never called (B-17); now `doProcessing` calls it after every chunk, and it drops a voice once it is not fading (`rampingDown`) and its env 1 `env` and `cenv` are both below -80 dB. Voice stealing (`POLYCOUNT` 32) counts only live voices.

## Things that are shared vs per voice
- Global (one copy): `controls[GPARAMS]` (float, normalised/recalculated control values), `ctrlValues[NPARAMS]` (int step values, set by `PMSG_STEP_SWITCH` messages), `rampedParams[]`, `EnvLfoCtrls[7]` (settings object, `Env` = `CLFO` with extra fields).
- Per voice (`RVoice`): 2 oscillators, SEM filter, 7 `CLFO` (index 0 = constant 0 "off", 1-3 envelopes, 4-6 LFOs), `envBank[7]` pointers to each `cenv`.
- Envelope attack/decay/release of a voice are **copied at note-on**, so turning those knobs affects only new notes. LFO rate/level are read every sample.

## Memory/cost notes
- `RVoicesMap` is allocated in `DXGenerator::init()` and never freed (destructor frees `ddMap`, log-scale objects and `doRecalc`, not the voices): B-24.
- Before the B-17 fix voices never left `notes`, so per-sample cost grew with the number of *distinct pitches played* (up to 128 voices, each doing two oscillators, filter, 3 LFOs, a log-scale lookup). Measured in `tests/note_off`: 5 s render after 60 pitches took 2328 ms before, 1078 ms after.

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
