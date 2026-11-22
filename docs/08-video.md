# Video material (film tool)

Started 2026-11-21. Everything here is for making promo video; none of it is in the installer or the
release package, and the plugin behaves the same with or without it.

## What there is

| Part | Where | Status |
|---|---|---|
| Film tool: MIDI in; editor layers, WAV and per-frame data out | `video/film/` (`film.cpp`, `midiFile.h`, `build.bat`) | step 1, working |
| Demo MIDI (4 bars, 120 BPM, bass, chords, melody, mod wheel) | `video/film/demo.mid` (from `make_demo_mid.py`) | |
| Blender test shot (UI capture on a plane, camera push-in) | `video/test01/pushin.py` | test only |
| Blender importer (layers on planes, data as keyframes, audio) | `video/blender/import_film.py` | step 2, working |
| Record-for-video (log a DAW export, replay it in the film tool) | behind a compile switch, never in a release | step 3, not started |

Renders go to `video/out/` (ignored by git).

## Plugin touchpoints (always compiled, no change in normal use)

- `source/uiClock.h`: every time-based editor animation reads `uiClock::now()`; it is `steady_clock`
  unless the film tool sets a manual time (`setManualSeconds`).
- `NVGView::advanceFrame()` / `drawScene(layers)`: `drawOpenGL` split in two. `advanceFrame` moves the
  frame-based animations on once per frame; `drawScene` draws the chosen layers (`kLayerBase`,
  `kLayerDisplays`, `kLayerArt`, `kLayerControls`, `kLayerOverlays`, `kLayerWindows`) and moves
  nothing on while the clock stands still. The editor calls both with all layers.
- `NVGView::makeGLCurrent()`: the film tool draws with the editor's GL context into its own framebuffer.

## Using the film tool

1. Build the plugin (Release|x64, as usual), then `video\film\build.bat`. Rebuild the tool after every
   plugin change: it links the plugin's object files.
2. `video\film\bin\film.exe song.mid --preset "Teardrop-1" --fps 30 --scale 3` (options: `film.exe --help`).
   It needs the licence installed on the machine (the same check the plugin makes; no bypass).
3. Output, in `video\out\film\<midi name>\` (or `--out`):
   - `base/ displays/ art/ controls/ overlays/` with `00000.png` ...: one folder per layer, PNG with
     straight alpha, bottom to top in that order. `--layers all` gives one folder with the whole editor.
   - `audio.wav`: the synth part, float32 stereo, from `--start`.
   - `data.json`: per frame: `time`, `song.*` (seconds, quarters, BPM), `audio.peak` / `audio.rms`,
     `midi.notesHeld`, `note.<pitch>.on/velocity`, `voice.<pitch>.level/held`, `playhead.<0..3>.*`
     (the four newest voices: pitch, age, level and their env1..3 / lfo1..3 levels), and
     `param.<id>.<name>` for every parameter. Channels that never change are under `constant`.

Frame N is at `start_seconds + N / fps`. Line the WAV (or the full mix from the DAW, exported from the
same start) up at frame 0.

- One track's MIDI: `--track N` / `--channel N`. The tempo map and time signature come from the file.
- `--start` / `--end`: the part to film; the tool pre-rolls from 0 so held notes and controllers are in place.
- Knob automation that a MIDI file doesn't carry: `--automate ID:T0:V0:T1:V1` (normalized value ramp,
  song seconds; IDs in `source/constants.h`, e.g. 42 = FILTER). Mod wheel, pitch bend and aftertouch in
  the MIDI file go through the plugin's MIDI mapping, as in a host.

## How it works

Per frame: render audio in blocks of up to 256 samples to the frame's sample position (with tempo and
song position), call `BaseProcessor::onTimer` once (scope, voices, playheads and peak reach the editor),
seed `rand()`, set `uiClock` to the frame time, `advanceFrame()`, then each layer: bind the framebuffer,
`nvgBeginFrame(vg, 1248, 542, scale)`, `drawScene(layer)`, read back, un-premultiply, save PNG (WIC).
The window's message loop does not run while frames are made, so nothing draws on wall-clock time.

Things found while building it:
- The (modded) `Win32OpenGLView::swapBuffers` releases the GL context (`wglMakeCurrent(dc, 0)`); the
  tool makes it current again after the one `drawOpenGL` it calls.
- The oscillators' reset reseeds `rand()` from the clock on every note (`srand(time(NULL))`,
  `Oscillator.cpp`, `LFOscillator.cpp`, `CLFOscillator.cpp`). In a host that is the audio thread
  (per-thread `rand()`); in the tool audio and editor share a thread, so the tool seeds `rand()` per
  frame for the readout's random letters. Anything in the engine that uses those `rand()` values for
  sound is time-seeded and won't repeat exactly between runs (not seen with the demo).
- The CONTROL readout reveals and fades a step per drawn frame; the tool starts it empty.

Checked (2026-11-21, demo.mid, 2 s, 30 fps, scale 2, FILTER automated 0.1 -> 0.9):
- two runs give byte-identical files (302: PNGs, WAV, data);
- the five layers stacked (straight-alpha over) equal the one-pass `--layers all` frames within 2/255;
- the WAV has 96000 samples (2 s at 48 kHz); the FILTER knob turns in the controls layer and
  `param.42.FILTER__1` ramps in the data;
- at scale 3 (3744 x 1626) NanoVG text and shapes are sharp; bitmaps (logo, panel art) are soft.

Speed: about 16 PNGs per second at scale 2 (5 layers x 30 fps: about 10 s per second of video), about
7 per second at scale 3.

## Blender importer

`blender -P video/blender/import_film.py -- video/out/film/NAME` opens Blender with the film in the
scene (`-b ... --save shot.blend` does it without a window; from the Text editor set `FILM_DIR` and run).
It adds, in a collection named after the folder:
- `UI`: an empty with one plane per layer, each an image sequence (straight alpha, emission, so the UI
  shines like a screen). 2 m wide (`--width`), standing up facing -Y, `--gap` metres apart (default
  2 mm), base at the back. Move or animate the planes to pull the layers apart.
- `Film data`: an empty with every data channel as a custom property keyed on every frame
  (`["audio.rms"]`, `["note.60.on"]`, `["playhead.0.env1"]`, `["param.42.FILTER__1"]`; on/held/pitch/
  velocity channels hold their value between frames, the rest are linear). Constants are plain
  properties. Use them through drivers: right-click a value > Add Driver, variable type Single Property,
  ID the `Film data` object, path `["audio.rms"]`.
- `audio.wav` as a sound strip at the first frame; the scene's fps and frame range (film frame N is
  scene frame `--frame-start` + N, default 1); an orthographic `Flat camera` if the scene has none.

Colour: the default (sRGB textures, Standard view) blends the layers' soft edges and glows in linear
light, a little brighter than the plugin, where NanoVG blends sRGB numbers. `--raw` (Non-Color textures,
Raw view) blends as the plugin does, but then lights and other objects in the scene work on sRGB numbers
too. A one-pass film (`--layers all`) is exact either way.

Checked (2026-11-22, Blender 5.2, the 2 s film above, flat camera at the film's size, pixel filter off):
- frame 31 matches film frame 30 best (frame mapping); flat colours exact, no pixel offset;
- default: mean 7/255 brighter than the plugin's stack, 0.2/255 from the same stack blended in linear
  light; `--raw`: every pixel within 1/255;
- all 3780 keyed values equal the data within 1e-7, the 190 constants are there, the sound strip runs
  frames 1-60, and a driver on `["audio.rms"]` reads the frame's value. The import takes about 2 s.

Not covered yet: the about/registration windows (`kLayerWindows`) are never filmed; the scope and preset
bar are in the art layer; hover effects (no mouse in the tool).
