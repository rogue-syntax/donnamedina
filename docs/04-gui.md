# GUI (pass 4, in progress)

Read, not run. Only the editor lifecycle has been traced so far.

## Layout (2026-11-13)
- Editor size and art placement live in `source/editorLayout.h` (`editorLayout::kEditorWidth/Height`, `kArtOffsetX/Y`, `artX()/artY()`). The window is 1248x542.
- The panel art (`resources/bkg1.png`, 1148x542, bitmap `BKG1.png`) is drawn at (-35,-15) by `NVGView::drawBkg`; the about art (`dm_about_window.png`) the same way. `NVGView::drawGrid` fills the area outside the art with the art's 5 px grid. It must stay outside the art: the art has transparent windows that the envelope/LFO displays (drawn before the background) show through.
- Control positions in `vstgui.uidesc` are window coordinates; all controls in template `basicControlls1` were moved by (-35,-15) at the same time. The `#mainRowBkg` and `#dmAboutWindow` views only hold bitmaps; their positions are not used.
- Positions written against the art in `NVGView` (envelope/LFO display origins, beat-mod dot, CONTROL readout text, BUILD INFO, registration box, about text) go through `artX()/artY()`.
- If the art is re-exported at window size with the panels moved, set the offsets to 0.
- `tests/editor_cycle`: `EDITOR_SHOT=<file.bmp>` opens the editor on screen once and saves a screenshot (the registration overlay is turned off for it, as the test has no processor).

## Creation chain
`DX10Controller::createView` -> `RSView` (VST3Editor subclass, uidesc `vstgui.uidesc`) -> uidesc asks for sub-controller `nvgViewController` -> `DX10Controller::createSubController` (`mdaDX10Controller.cpp:79`) does `new NVGViewController(editor, this)`, whose ctor does `new NVGView(CRect(0,0,kEditorWidth,kEditorHeight), controller)`. `NVGViewController::createView` returns that `nvgView` when the uidesc view has `custom-view-name="nvgView"`. So **one new NVGView per editor open**.

## NVGView
- `NVGView : COpenGLView` draws the whole UI with NanoVG into one GL view (1248x542 since 2026-11-13, see Layout below). The normal VSTGUI controls (modded `CBKnob`, `CBButton`, `CBSlider`, `CBVerticalSwitch`, `CBStepCtrlButton`) still exist as hidden/transparent input handlers; their look is drawn by NanoVG.
- `attached()` (`NVGView.cpp:934`): create GL3 NanoVG context + font; walk **all views in the frame** (`getFrame()->getViewsAll`, an SDK mod), and for each one whose `customRSType > -1` call a `setupX` function by type number: 1 knob, 2 button, 3/10 slider, 4 vertical switch, 5 vertical slider, 6 osc-symbol (name decides sine/saw/tri/square), 7 knob2 (via setupRKnob), 8 image, 9 click button, 11 step-ctrl button, 12 knob2, 13 more-info, 14 close-window button. Each `setup` builds an `RSBView` data record, stores it in `RViewsMap`, indexes it by control tag in `viewIndex[tag]`, and registers a draw handler in `RDrawVector[layer]` (the `Draw`/`REvent` data+binder system).
- `drawOpenGL()` (`:1391`) every frame: `onNVGViewReady()` flush, animations, draw envelopes/LFOs, then each registered draw handler, then overlays; `glFinish(); invalid();` (continuous redraw).
- `removed()` (`:1137`): clears flag, makes the GL context current and deletes `vg` (B-28 fix; `nvgDeleteGL3` now also frees the font stash again, B-54), `views` cleared. VSTGUI then frees the `NVGView` and deletes the `NVGViewController`, which clears `DX10Controller::nvgViewController` (B-29). Measured with `tests/editor_cycle`.
- Event/data system `revent/`: custom `Map.h` hash (ska) of name -> data + member-function binders. Older `removedFromSrc/REvent*.{h,cpp}` is the earlier version. Notes at `C:\vst3sdk\revents.txt`.

## Open (not read yet)
Draw functions (`drawEnv1-6`, `drawKnob`, ...), animation machinery, `RandyTxt`, `onChangeOSCType`/step-control handlers (~l.107-380), `setupRSlider*`/`SwitchVert`, the `revent` data system internals, `RSOGProc/View` liveness, SDK/VSTGUI diff.
Findings so far: B-28, B-29, B-30 in `BUGS.md`.

## Pass 4b: handlers, text effect, knob drawing (read)
- `onEnv/onEnvBipolar/onPanEnv/onEnvLev/onTuneSwitch/onOnOffBtn/onEnvSrc/onChangeOSCType/onChangeLFOOSCType/onChangeModType` (`NVGView.cpp:52-268`) only update the on-screen readout: `targDisplayTxt` (parameter name) and `targValueTxt` (value text) via `RandyTxt::reset`. `onChangeModType(3)` also sets `beatModOn`, which drives the beat-mod indicator. They never touch audio params.
- `onResetButton` (`:271`): `idx = pid - WIDTH_RESET` indexes `resetSwitch[7] = {WIDTH_, TUNE_, FINE_, MOD_GAIN_, MOD_WIDTH_, MOD_TUNE_, MOD_FINE_}` (reset pids 150..156); sets the target control to 0.5 normalised and calls its `valueChanged()`, plus a blink animation. So a "reset" is just the GUI control being set to centre, which then goes through the normal parameter path.
- `onStepCtrlSwitch`/`onStepCtrlButton`/`onToggleButton`: set blink/glow animation entries in `animationReel` (+ `stepCtrlSelectAnim`).
- Animations are plain structs (`Anima`: name, control index, state) in `animationReel`; `drawOpenGL` runs each through `nvgAnimations[name]` (member-function table: `buttonBlink`, `buttonBlinkFastest`, `buttonGlow`, `stepTargBlink`) until it returns state 1. The VSTGUI `animationTick/Start/Finished` overrides are marked "NOT USED".
- `RandyTxt`: the scrambling-letters readout. `randyCharSrc` is 32 characters, matching `rand() % 32`. Per-frame timers (B-35).
- `drawKnob`: not an image knob; draws a circle outline, a magenta fill whose alpha = `alpha * blinkIntensity`, and a line from centre to `cBKnob->valueToPoint`. **The pointer is drawn from the VSTGUI control's own value each frame**, so the display only ever shows the control's state; it is correct only if `setViewsFromBase`/`doFromBase` kept the controls in sync (links to B-02).
- New findings: B-33, B-34, B-35.
Still unread: `drawEnv1-6`, slider/switch/button/osc-symbol drawing, `drawBkg`, `drawOGL`, more-info window, `REvents`/`RData`/`Map.h` internals.

## Pass 4c: draw functions and revent (read)
- `drawEnv1-3`: envelope shape polygon from the attack/decay/release knob positions and the decay-toggle button (approximation of the sound, not the real coefficients). `drawEnv4-6`: LFO preview from type/amp knobs, a phase slider, and the rate value pushed from the processor (`lfoNRate` pointers into the controller, via `PMSG_LFO4/5/6_RATE`); number of cycles drawn tracks rate. Names are hard-coded (B-38).
- `drawSlider/SliderVert`: bipolar fill from centre by `value - 0.5`. `drawSwitchVert`: one tick per step (`getMax()+1`) and a magenta marker at the current value. `drawSelctedStepTarg`: outline round-rect (RSBType 1) or circle (2) around the step-control target; dereferences `selectedStepTarg` unguarded (set in `attached`).
- **revent** (`REvents.h`): a registry class owning (a) events + binders (queue and `execQueue`; apparently unused by the GUI), (b) `RDataMap` name->`RData` and `boundHandlers` handle->`{dataName, member fn}`, run by `execDataHandler(handle)` each frame, (c) `functionMap` name->member fn, run by `callFunction`; this is how `attached()` dispatches `setupRKnob` etc. by string name, (d) a voice binder set for a former RVoice-event design (commented out in the newest copy). Pointer-to-member of `RDataConsumer` bound to the `NVGView` instance. Hash map = ska (`Map.h`, `ska.h`).
- Build/dependency issue: B-36. Cost notes: B-37.
Still unread: `drawBkg`, `drawOGL`, `drawButton/CButton`, `drawSineSymbol`, `drawImage`, more-info window and its button handlers, `RSOGProc/RSOGView` (only referenced via `NVGView.h:43` include and an unused `setupViewsMap`, so very likely dead).
