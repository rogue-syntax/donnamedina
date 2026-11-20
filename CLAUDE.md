# Donna Medina VST3 synth (RSSynth001)

C++ VST3 instrument inside a modded Steinberg VST3 SDK (`C:\vst3sdk\VST3_SDK`). Built with Visual Studio 2022 v143 (`RSSynth001donnaMedina.sln`, Debug|x64 and Release|x64 only); see `BUILDING.md`. Git repo at the project root (remote `rogue-syntax/donnamedina`); the SDK itself is not under version control.

Read first: `docs/00-overview.md` (what is live vs dead), `docs/01-params-and-state.md` (param/message/preset/state flow), `docs/02-voices-and-dsp.md` + `docs/03-modulation.md` (engine), `docs/BUGS.md` (ledger with verification log). `docs/05-sdk-mods.md` lists the SDK/VSTGUI customizations.

Rules of thumb
- Live plugin = `rs::DX10Processor` / `rs::DX10Controller` (`source/mdaDX10*`) driving `DXGenerator`, which renders the `RVoice`s in groups of 4 (AVX2) through `renderBlock` / `renderBankGroup` (`source/DXGeneratorBank.cpp`, components in `source/VoiceBank.h`); see `docs/02-voices-and-dsp.md`. `VSTSynthProcessor/Controller` (+ `Voice`, `RSSynth002Voice`) are dead and no longer compiled (files kept). VST3 only (VST2 export removed).
- Param IDs/constants live in `source/constants.h`; GPARAMS=1000 sizes state and dispatch tables.
- Author notes live in `C:\vst3sdk\*.txt` (todos, issues, customviewNotes, revents).
- Statements in docs marked "unverified" come from reading code only.
- Edits to SDK files affect all projects; log them in `docs/05-sdk-mods.md`.
- Build check: `MSBuild RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64 /p:PostBuildEventUseInBuild=false` (the post-build step copies into Program Files).
- Dependencies: `external/` (GLEW, Monocypher; libbcrypt is kept but no longer built) and `VST3_SDK/external/revent`. Never add absolute paths.
- Licensing: signed `RS1.` tokens, `source/license.*`; update check `source/updateCheck.*`; installer `installer/` (store recipe, `installer\build.bat`). Spec: `docs/store/licensing-v1.md` (verbatim copy of the store's master; don't edit), how we meet it: `docs/07-licensing.md`. No bypasses or machine-ID licence checks; the developer uses a `kind=dev` license. Tests: `tests\license\build_and_run.bat`, `tests\update\build_and_run.bat`. The product is spelled "Donna Medina" (no tilde).
