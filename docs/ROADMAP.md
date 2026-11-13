# Roadmap: GUI makeover and features

Ideas collected 2026-11-13 after the engine/bug-fix passes. Status: `[ ]` open, `[x]` done, `[~]` in progress.
Order within a section is a rough priority (impact for effort).

## 0. Make room
- [x] Move all controls -35,-15 to remove the dead frame space; draw the 1148x542 art at that offset in code;
      widen the editor to 1248 (grid fills the new space). `source/editorLayout.h`, `resources/vstgui.uidesc`.
- [ ] Design the extended right column (≈170 px of new space right of the logo/pan/control column).
- [ ] Optional: re-export the background art at 1248x542 with the panels moved, then set the art offset to 0.
- [ ] Optional: use or trim the ≈24 px now free at the bottom.

## 1. GUI: high-value
- [ ] **Live readout in CONTROL/VALUE**: the parameter under the mouse / being turned, with real units
      (Hz, ms, semitones, cents, tempo divisions). Today it shows names and raw values only for some controls.
- [ ] **Animated LFO displays**: real waveform shape, shaded delay section, a moving phase dot from the
      most recently played note.
- [ ] **Envelope playheads**: a dot travelling along each envelope shape for the latest notes (pulses with
      rhythmic playing).
- [ ] **Modulation visibility**: colour per source (Env 1/2/3, LFO F/G/H) tinting the knobs/squares it drives,
      and/or a ring around knobs showing the live modulated value.
- [ ] **Preset bar**: name, prev/next, save, randomize (no preset browser is visible today).
- [ ] **Voice strip**: a 128-key strip lighting each per-note voice as it plays and fades.
- [ ] **Scope or spectrum** in the BUILD INFO box (NanoVG/GL already running).
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
