# Roadmap: GUI makeover and features

Ideas collected 2026-11-13 after the engine/bug-fix passes. Status: `[ ]` open, `[x]` done, `[~]` in progress.
Order within a section is a rough priority (impact for effort).

## 0. Make room
- [x] Move all controls -35,-15 to remove the dead frame space; draw the 1148x542 art at that offset in code;
      widen the editor to 1248 (grid fills the new space). `source/editorLayout.h`, `resources/vstgui.uidesc`.
- [x] Right column remake: one framed column, bands for title / preset / readout / scope / master / voices / logo.
  - [x] Step 1: layout and frames, pan knobs moved to the master row, logo is the about button.
  - [x] Step 2: readout with units, on turn and on hover (`paramDisplay`, docs/04-gui.md "Readout").
  - [x] Step 3: scope (triggered output waveform, docs/04-gui.md "Scope").
  - [x] Step 4: voice strip, then moved into the scope with a zooming pitch range (docs/04-gui.md "Voices in the scope").
  - [x] Step 5: preset bar (docs/04-gui.md "Preset bar").
  - [ ] Optional: transparent title/logo PNGs (2x) to replace the crops from the art.
  - [x] Master VOL knob: new parameter `MASTER_VOL` (146), global (not in presets), 0 dB default; see docs/01-params-and-state.md.
- [ ] Optional: re-export the background art at 1248x542 with the panels moved, then set the art offset to 0.
- [ ] Optional: use or trim the ≈24 px now free at the bottom.

## 1. GUI: high-value
- [x] **Live readout in CONTROL/VALUE**: the parameter under the mouse / being turned, with real units
      (Hz, ms, semitones, cents, tempo divisions). Done in column step 2.
- [x] **LFO displays**: the LFO after a note-on on a time axis (real rate, amplitude, start phase, hatched
      delay), not animated, by your call (docs/04-gui.md "LFO displays"). Playhead dot: with the envelope playheads.
- [ ] **Envelope playheads**: a dot travelling along each envelope shape for the latest notes (pulses with
      rhythmic playing).
- [ ] **Modulation visibility**: colour per source (Env 1/2/3, LFO F/G/H) tinting the knobs/squares it drives,
      and/or a ring around knobs showing the live modulated value.
- [x] **Preset bar**: name with a preset menu, prev/next, save (.vstpreset), randomize (column step 5).
- [x] **Voice view**: each per-note voice as a bar behind the scope, zoomed to the sounding range (column step 4).
- [x] **Scope** in the right column (column step 3). A spectrum view could be added later as a toggle.
- [ ] Labels or hover tooltips for the unlabeled top knob rows and the small squares.

## 2. GUI: polish
- [ ] Knob value arcs (glowing track from min to value) in addition to the pointer.
- [ ] Glow/bloom on outlines when active (sounding section, sequencer step lit).
- [ ] Match the A–H step-target letters with coloured badges on the controls they drive.

## 3. Sound and engine
- [ ] **Snap / retrigger-time knob**: expose the fixed 21 ms self-cut fade (≈2 ms punchy … ≈80 ms smooth).
- [ ] **Per-note expression** (VST3 Note Expression / MPE): bend, volume, pan, brightness per note; fits the
      one-voice-per-pitch design.
- [ ] **HQ mode**: 2x oversampling to reduce FM aliasing on high notes (affordable after the 3x speedup).
- [ ] **Per-note analog drift**: small random pitch/filter/pan offset per note.
- [ ] **Velocity routing** to filter and mod gain (check what velocity already reaches first).
- [ ] **Raise POLYCOUNT** from 32 to 64/128 (one line; cost is linear).
- [ ] **FX strip**: tempo-synced (ping-pong) delay and/or chorus.
