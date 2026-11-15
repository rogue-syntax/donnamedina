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
      delay), not animated, by your call (docs/04-gui.md "LFO displays"), with playhead dots.
- [x] **Playheads**: a dot per recent voice (newest 4) travelling along each envelope shape and LFO display
      (docs/04-gui.md "Playheads").
- [x] **Modulation visibility**: a colour per source, rings on the source/amount knobs, range and live
      value arcs on the destination knobs (docs/04-gui.md "Modulation colours and rings").
- [x] **Preset bar**: name with a preset menu, prev/next, save (.vstpreset), randomize (column step 5).
- [x] **Voice view**: each per-note voice as a bar behind the scope, zoomed to the sounding range (column step 4).
- [x] **Scope** in the right column (column step 3). A spectrum view could be added later as a toggle.
- [~] Labels or hover tooltips for the unlabeled top knob rows and the small squares: the squares (reset) and both
      knob rows name themselves in the CONTROL readout on hover; the colours show which source a column uses.

## 2. GUI: polish
- [x] Knob value arcs on the outline (from the centre for bipolar knobs) (docs/04-gui.md).
- [x] Glow when active: each envelope/LFO display glows in its source colour with the newest voice's level while
      the source is routed (docs/04-gui.md). Step buttons already flash when pressed by host or GUI; left as is.
- [x] A–H step-target badges: the selected target's dial letter and control letter lit as a pair (docs/04-gui.md).

## 3. Sound and engine
- [x] **Snap**: SNAP_TIME 2..80 ms (default 21) on the new PLAY panel; state version block for new parameters.
- [ ] **Per-note expression** (VST3 Note Expression / MPE): bend, volume, pan, brightness per note; fits the
      one-voice-per-pitch design.
- [x] **HQ mode**: 2x oversampling toggle (HQ in the PLAY title row), ~1.95x voice CPU; fixed B-63 on the way.
- [x] **Drift**: per-note random pitch (+-15 cents), filter and pan offsets, DRIFT knob on the PLAY panel.
- [x] **Expression**: one bipolar knob offsetting BRIGHT / FILTER / FM DEPTH / WIDTH, optionally scaled by velocity (VEL).
- [x] **Polyphony control**: 1..64 voices (POLY_COUNT, global), on the scope's VOICES plate; steal order released -> quietest.
- [ ] **FX strip**: tempo-synced (ping-pong) delay and/or chorus.
