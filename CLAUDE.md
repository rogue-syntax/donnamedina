# Donna Medina VST3 synth (RSSynth001)

C++ VST3 instrument inside a modded Steinberg VST3 SDK (`C:\vst3sdk\VST3_SDK`). Built with Visual Studio 2022 v143 (`RSSynth001donnaMedina.sln`, Debug|x64 and Release|x64 only); see `BUILDING.md`. Git repo at the project root (remote `rogue-syntax/donnamedina`); the SDK itself is not under version control.

Read first: `docs/00-overview.md` (what is live vs dead), `docs/01-params-and-state.md` (param/message/preset/state flow), `docs/02-voices-and-dsp.md` + `docs/03-modulation.md` (engine), `docs/BUGS.md` (ledger with verification log). `docs/05-sdk-mods.md` lists the SDK/VSTGUI customizations.

Rules of thumb
- Live plugin = `rs::DX10Processor` / `rs::DX10Controller` (`source/mdaDX10*`) driving `DXGenerator`, which uses `RVoice` / `WTOscillatorVoice`. `VSTSynthProcessor/Controller` (+ `Voice`, `RSSynth002Voice`) are dead and no longer compiled (files kept). VST3 only (VST2 export removed).
- Param IDs/constants live in `source/constants.h`; GPARAMS=1000 sizes state and dispatch tables.
- Author notes live in `C:\vst3sdk\*.txt` (todos, issues, customviewNotes, revents).
- Statements in docs marked "unverified" come from reading code only.
- Edits to SDK files affect all projects; log them in `docs/05-sdk-mods.md`.
- Build check: `MSBuild RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64 /p:PostBuildEventUseInBuild=false` (the post-build step copies into Program Files).
- Dependencies: `external/` (GLEW, libbcrypt) and `VST3_SDK/external/revent`. Never add absolute paths.
