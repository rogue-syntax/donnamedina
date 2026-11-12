# Building Donna Medina

## Requirements
- Windows, Visual Studio 2022 with the **v143** toolset (Desktop C++ workload) and Windows SDK 10.0.22621.
- The modded VST3 SDK at `C:\vst3sdk\VST3_SDK` (this repo must sit at `VST3_SDK\public.sdk\MyProjects\SynthProjects\Windows\RSSynth001donnaMedina`; paths to the SDK are relative).
- The SDK's CMake build folder `VST3_SDK\build` (generated with "Visual Studio 17 2022", x64). The solution builds its `base`, `sdk`, `vstgui*` libraries.
- `VST3_SDK\external\revent` (shared with the modded VSTGUI `cbuttons.h`).

Everything else (GLEW, libbcrypt) is in `external/` in this repo. Nothing is read from `C:\D` any more.

## Build
Open `RSSynth001donnaMedina.sln`, pick **Debug|x64** or **Release|x64**, Build.
Command line: `MSBuild RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64`

Output: `x64\<Config>\RSSynth001donnaMedina.vst3` (Debug also writes `DonnaMedinaVST64.pdb`).

## Deploy (post-build, every build)
Copies the plugin to `%CommonProgramW6432%\VST3\DonnaMedinaVST64.vst3` (Debug also copies the `.pdb`). Writing there needs Visual Studio **run as administrator**, and the host must not have the plugin loaded; otherwise the build fails with "Deploy failed".
- Build without deploying: add `/p:PostBuildEventUseInBuild=false`.
- Deploy somewhere else: `/p:CommonProgramW6432=D:\SomeFolder` (the plugin goes to `D:\SomeFolder\VST3\`).
- Quick build + install without Visual Studio: run `build_and_install.bat` (double-click or any prompt). It asks for admin rights, builds Release|x64, keeps the first installed plugin once as `DonnaMedinaVST64.vst3.original`, and copies the new build over it. Close the host first.

## Registration
Without the registry value `HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina\RSRegKey` the plugin loads but plays no notes (see `docs/00-overview.md`).
