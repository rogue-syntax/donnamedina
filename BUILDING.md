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

### Debug vs Release
- **Debug**: no optimisation, debug C runtime (`/MDd`). For stepping and asserts only: several times slower, and it needs Visual Studio's debug runtime DLLs, so it does not load on users' machines. Never ship it.
- **Release** (2026-11-18): `/O2`, favour speed, fast floating point, **AVX2** (`/arch:AVX2`), whole-program optimisation (`/GL` + `/LTCG`), function-level linking with unreferenced-code removal and identical-code folding (`/Gy`, `/OPT:REF`, `/OPT:ICF`), parallel compile (`/MP`).
  - **AVX2 is a hard requirement**: the plugin crashes on a CPU without it (Intel before Haswell, 2013; AMD before Excavator, 2015). State it in the system requirements.
  - **Needs the Microsoft Visual C++ 2015-2022 Redistributable (x64)** (`/MD`, the runtime is a DLL). Bundle it with the installer or list it as a requirement. Linking the runtime statically (`/MT`) would need the SDK libraries in `VST3_SDK\build` built `/MT` too.
  - Measured with `tests\note_off\bin\note_off_test.exe perf 1 8 32` (5 s of held notes, default patch) and checked with `golden`:

    | Release flags | 32 voices, % of one core | ns per voice-sample | 32 voices HQ | DLL size |
    |---|---|---|---|---|
    | before (`/O2` only) | 7.9 % | 56 | 15.5 % | 4.73 MB |
    | + `/GL` `/LTCG` `/OPT:REF,ICF` | 7.9 % | 56 | 15.4 % | 3.67 MB |
    | + AVX2 | 7.6 % | 54 | 15.3 % | 3.72 MB |

    `/GL` changed no output sample; AVX2 changes the output by at most 7e-5 (113 dB below the signal: fused multiply-add rounding). The voice loop (`DXGenerator::singleVoiceProcessing` and the oscillators, ~80 % of the time) is scalar, one voice and one sample at a time, so the compiler cannot vectorise it; bigger gains need that loop restructured (several voices per SIMD register), not flags.

## Deploy (post-build, every build)
Copies the plugin to `%CommonProgramW6432%\VST3\DonnaMedinaVST64.vst3` (Debug also copies the `.pdb`). Writing there needs Visual Studio **run as administrator**, and the host must not have the plugin loaded; otherwise the build fails with "Deploy failed".
- Build without deploying: add `/p:PostBuildEventUseInBuild=false`.
- Deploy somewhere else: `/p:CommonProgramW6432=D:\SomeFolder` (the plugin goes to `D:\SomeFolder\VST3\`).
- Quick build + install without Visual Studio: run `build_and_install.bat` (double-click or any prompt). It asks for admin rights, builds Release|x64, keeps the first installed plugin once as `DonnaMedinaVST64.vst3.original`, and copies the new build over it. Close the host first.

## Registration
Without the registry value `HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina\RSRegKey` the plugin loads but plays no notes (see `docs/00-overview.md`).
