# Note event trace: host event -> voice (pass 5)

Read from code; timing numbers come from a small Python model of the envelope/retrigger logic (my reading of the code, **not** the real build; model script was a scratch file). Host behaviour not observed.

## 1. Event path
`processEvents` (`mdaDX10Processor.cpp:~1230-1354`) copies kNoteOn/kNoteOff into `notes[]` as triples `{sampleOffset, pitch, velocity*127 (note-off = 0)}`, sorts them (`sortEventsByOffset`) and `preProcess` renders chunk-by-chunk, calling `DX10Processor::noteOn(note, vel, id)` for each event at its offset (`vel == 0` means note-off; the **same function** handles both and is gated by `regKeyLegit`).
- Sort rule: ascending offset; for equal offsets, **note-on is moved before note-off** (the comment says "note-offs come after note-ons"). Host order for back-to-back same-pitch notes is normally off then on, so the sort reverses it.
- Overflow branch (more events than `EVENTBUFFER`): note-offs first, then note-ons in reverse order.
- A real note-on with velocity so small that `velocity*127` truncates to 0 would be treated as a note-off (edge case).

## 2. Voice side
- Voice index = MIDI pitch (`RVoicesMap[pitch]`). `notes` (active list) never shrinks (B-17), so **every pitch played once stays in `notes`**.
- `noteOn`, pitch already in `notes` (i.e. any repeat): `retriggerVoice(v, false)`: `env = cenv; catt = 1; rampingDown = true; envInc = -(1000*env)/21/sr`. The old tail is faded linearly over ~21 ms (only if it is still loud), then `interpolateVoice` calls `processNoteOn`, which re-initialises all envelopes (including `cdec`) from the global settings.
- `noteOff`: sets `cdec = crel` (or leaves decay), `env = cenv`, `catt = 1`. It does not know about `rampingDown`.
- At the end of the fade `processNoteOn` overwrites what `noteOff` set, so **a note-off that arrives while `rampingDown` is true is erased** (B-18).

## 3. When does the fade actually run?
Only if the previous note on that pitch is still audible: the fade ends as soon as `env < 0.0003`, and `retriggerVoice` starts it from the current `cenv`. If the pitch has fully decayed the fade is skipped (first tick restarts the note, so a following note-off is applied normally). So the lost note-off needs: same pitch played before **and still audible** (long release or hold) **and** key-up within ~21 ms of the new key-down.

## 4. Model results (44.1 kHz; `cenv` after 0.1, 0.5, 1.0, 2.9 s; previous note released 50 ms before the replay, release=0.5)
| scenario | decay = HOLD (>0.991) | decay = 0.6 |
|---|---|---|
| replay, note-off at 200 ms | 0.99, 0.03, 0.0001, 0 (normal) | 0.49, 0.004, 0, 0 |
| replay, note-off at 10 ms (inside fade) | 0.99, **1.0, 1.0, 1.0 (hangs forever)** | 0.49, 0.006, 0, 0 (decays by decay stage, gate ignored) |
| replay, note-off at 30 ms (after fade) | 0.20, 0.0015, 0, 0 (normal) | 0.21, 0.0016, 0, 0 |
| host `off, on` at same sample (sorted to `on, off`), key then held | stays at 1.0 (correct for a held key) | decays by decay stage |

Reading: the same-sample reorder (B-05) is **mostly harmless in itself** because the misplaced note-off lands inside the fade and is erased by B-18 (accidentally correct for a held key). The real harm is a **short gate (< 21 ms) on a repeated pitch whose previous note is still sounding**: with decay at hold the note never ends (ghost), with finite decay the gate/release is ignored. Fast arps/sequences (e.g. 1/32 notes at high tempo with short gate) on repeated pitches fit this; the "Brooding Sky Arp" preset is a natural test. I have not reproduced it in a host.

## 5. Still unexplained
"second voice drops out on retrig" is not explained by this (the retrigger path keeps the voice alive); "modulation drops out on retrig when env too high" remains unexplained (B-19 withdrawn).

## 6. "INIT modulation adds up on voice stealing" (pass 5 check)
- `processNewMainEnv()` (called on every note-on) is an **empty function**; `retriggerMainEnv()` too. Nothing there can accumulate.
- Steal path (`notesSize >= POLYCOUNT`): the quietest voice gets `retriggerVoice(..., true)` (21 ms fade via `deadVoiceProcessing`), the new pitch gets `processNoteOn`. `processNoteOn` calls `WTOsc.startOscillator()`/`WTOsc2.startOscillator()` which call `reset()`; `COscillator::reset()` zeroes `m_dFoModLin` and `m_dPhaseMod`, so modulation inputs do **not** carry over between notes.
- Things that *can* carry over: (a) in detune mode `deadVoiceProcessing` adds to `WTOsc2.m_dOscFo` every sample without recomputing it (B-47 note), harmless once the voice is reused; (b) B-47 below.
- Not found: any `+=` on persistent per-voice modulation state in the note-on path. This note stays OPEN without a repro; the likeliest literal reading ("init adds up") is not supported by the code.
- Conclusion: this note stays OPEN without a repro. The literal reading ("init adds up") is not supported by the code; the nearest real effect is B-47 (stale mod after changing mod type). Detune mode also adds to the modulator frequency every sample in `deadVoiceProcessing` (fades only, ~21 ms).
