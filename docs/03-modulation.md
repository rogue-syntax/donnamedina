# Modulation, envelopes, LFOs, smoothing (pass 3)

Read, not run. See `02-voices-and-dsp.md` for the voice signal flow.

## Sources ("env src" 0..6)
Every modulatable destination has an `X`, an `X_ENV` (amount, bipolar: `2*p - 1`) and an `X_ENV_SRC` (which source). The source index selects `voice.envBank[src]`, a pointer to a per-voice `CLFO::cenv`:

| src | source | notes |
|---|---|---|
| 0 | `voiceEnvLfo[0].cenv` | forced to 0 every sample = "off" |
| 1 | env 1 | also the **amp envelope** |
| 2 | env 2 | |
| 3 | env 3 | |
| 4 | LFO 1 | per-voice instance, bipolar `-level..+level` |
| 5 | LFO 2 | |
| 6 | LFO 3 | |

Effective value of a destination is basically `base + amount * source`, then clamped (examples: width `0.1..0.9`, gain `silenceCutoff..1`, filter `0.01..1` before the log-scale map, pan `-1..1`). Step-type destinations (interval, octave, osc/mod osc type, mod type, env src selectors) come from `ctrlValues[]` (ints delivered by `PMSG_STEP_SWITCH` messages), not from the normalised param.

## Destinations seen in `singleVoiceProcessing`
Osc1: interval (`INT_`/`INT_ENV` x13 semitones), octave (`OCT_`/`OCT_ENV` x6), tune+fine (`TUNE_*`, `FINE_*`), width. Osc2/modulator: same set with `MOD_` prefix plus `MOD_BEAT_` (detune mode). Mix: `MOD_GAIN_` (modulation depth), `MOD_OSC_GAIN` (modulator audible level), `GAIN_`, `FILTER_`, `FILTER_Q`, `PAN_LR_`. TUNE link: `SYNC_GEN_MOD_TUNE` toggle makes the modulator use the carrier's tune env source (`linkTune`, `processNoteOn` `:507-514`, `recalcSyncGenModTune`) and tells the controller (`CMSG_LINK_TUNE`) so the controller mirrors TUNE/FINE edits to the MOD_ params (`BaseController::performEdit`).

## Modulation types (`modChoice`, `MOD_TYPE_` step 0..3)
- 0 `doNoMod`: modulator rendered, not applied.
- 1 `doPhaseMod`: `WTOsc.m_dPhaseMod = clamp(MOD_GAIN_ ramped + MOD_GAIN_ENV*src, 0.003, 1) * osc2out`.
- 2 `doFreqMod`: `WTOsc.m_dFoModLin = clamp(...)*4 * (osc1Fo * osc2out)`.
- 3 `doDetuneMod`: shifts modulator freq by a tempo-based beat amount (`bpm/60 * (MOD_BEAT_ + env*13)/4`) and then like freq mod with `*8`.
Note `doDetuneMod` adds to `WTOsc2.m_dOscFo` every sample, but `singleVoiceProcessing` recomputes `m_dOscFo` from the table each sample first, so it does not accumulate in live voices (verified by reading order). **Exception:** `deadVoiceProcessing` (killed/stolen voices fading out) calls `modChoice` without recomputing `m_dOscFo`, so in detune mode the modulator frequency would keep growing during the ~21 ms fade (not verified audibly; minor).

## Envelope math (global settings in `DX10Processor::recalcEnvNxxx`, `:1483-1590`)
- Attack coefficient `catt = 1 - exp(-ifs * exp(8 - 8*v))`.
- Decay coefficient `cdec = exp(-ifs * exp(6 - 6*v))`; **`v > 0.991` => `cdec = 1.0` = no decay (sustain/hold)**.
- Release coefficient `crel = exp(-ifs * exp(5 - 5*v))`.
- `decayReleaseToggle` (< 0.5): on note-off use `crel` instead of `cdec`.
- These are applied by copying `catt/cdec` into the voice at `processNoteOn`.
So with decay at max (hold) and the toggle on, a note only ends on note-off with release. If a note-off is missed (B-18) the voice rings forever.

## LFOs
`CLFO` (derives from `CLFOscillator`): waveforms sine/saw/tri/square (`m_uWaveform` 0..3), per-voice phase reset at note-on (`moduloStart`/`phaseStart`), `m_dDelayTime` fade-in, saw has an anti-click "chase" ramp. Rate: either free (`cookVSTGUIVariable(MIN_LFO_RATE, MAX_LFO_RATE, v)`) or tempo-sync (`bpm / quants[round(v*10)]`) per LFO via `syncBpm`. `recalcLfoNRate` also sends the computed rate back to the controller (`PMSG_LFO4/5/6_RATE`) for display; see B-13 for the per-chunk resend problem.

## Smoothing (`ParamRampObj`)
`rampedParams[]` smooth GAIN, WIDTH, MOD_WIDTH, FILTER, PAN, PAN_ENV, MOD_GAIN, MOD_OSC_GAIN (`rampedList`, `GENRAMPEDPARAMSLENGTH`). `setLevels2(target, 100, &sr)` computes `inc = 1000/100/sr` (~0.00023 at 44.1 kHz) and then **clamps it up to `thresh` (0.002)** (`ParamRampObj.cpp:24-30`), so the real step is **0.002 per sample = full scale in ~500 samples (~11 ms at 44.1 kHz, ~5 ms at 96 kHz)**, not the 100 ms the argument suggests (corrected in pass 5; my earlier text said `inc` was much smaller than `thresh`, which ignored the clamp). `processLevel()` moves by `inc` while `|target - level| > thresh`, then snaps; since `inc == thresh` it cannot overshoot. The time is sample-rate dependent only through the clamp (fixed ~500 samples). `level` is uninitialised until `initLevel`, which `setCurrentProgram`/`setState` call.

## Controller-side step values
Step params are registered as `StringListParameter` entries whose strings are the numbers ("0".."n"). The controller converts a normalised value to the string, `safeStoi`s it, and sends it to the processor, which stores it in `ctrlValues[pid]` (B-23 for an out-of-range write on this path).
