# Building Donna Medina

## Requirements
- Windows, Visual Studio 2022 with the **v143** toolset (Desktop C++ workload) and Windows SDK 10.0.22621.
- The modded VST3 SDK at `C:\vst3sdk\VST3_SDK` (this repo must sit at `VST3_SDK\public.sdk\MyProjects\SynthProjects\Windows\RSSynth001donnaMedina`; paths to the SDK are relative).
- The SDK's CMake build folder `VST3_SDK\build` (generated with "Visual Studio 17 2022", x64). The solution builds its `base`, `sdk`, `vstgui*` libraries.
- `VST3_SDK\external\revent` (shared with the modded VSTGUI `cbuttons.h`).

Everything else (GLEW, Monocypher) is in `external/` in this repo. Nothing is read from `C:\D` any more.

## Build
Open `RSSynth001donnaMedina.sln`, pick **Debug|x64** or **Release|x64**, Build.
Command line: `MSBuild RSSynth001donnaMedina.sln /p:Configuration=Release /p:Platform=x64`

Output: `x64\<Config>\RSSynth001donnaMedina.vst3` (Debug also writes `DonnaMedinaVST64.pdb`).

Build stamp: a pre-build step (`tools\buildstamp.ps1`, PowerShell + git) writes `source\buildstamp.cpp` (not in git) with the branch, short commit and build time; the About window shows it in its info row, e.g. `BUILD simd-voices 448d975  2026-11-19 11:33`. The commit is the last one at build time: uncommitted changes in the working copy are not marked. Without git it says `unknown unknown`.

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
Copies the plugin to `%CommonProgramW6432%\VST3\DonnaMedinaVST64.vst3` (Debug also copies the `.pdb`), the file the host loads, so every build is the one you test. The host must not have the plugin loaded; otherwise the build fails with "Deploy failed" (close the project or the host and build again). Writing there needs write access to that folder: on the development machine the `Users` group has it; on a standard Windows install run Visual Studio **as administrator**.
- Build without deploying (only when a host must stay open): add `/p:PostBuildEventUseInBuild=false`. The host then keeps running the previous build.
- Deploy somewhere else: `/p:CommonProgramW6432=D:\SomeFolder` (the plugin goes to `D:\SomeFolder\VST3\`).
- Quick build + install without Visual Studio: run `build_and_install.bat` (double-click or any prompt). It asks for admin rights, builds Release|x64, keeps the first installed plugin once as `DonnaMedinaVST64.vst3.original`, and copies the new build over it. Close the host first.

## Licence
Without a valid licence (registry value `HKCU\Software\Rogue-Syntax\Registrations\DonnaMedina\License`) the plugin loads but plays no notes and shows "Not registered". On a development machine paste a `kind=dev` licence from the store owner (Paste license); see `docs/07-licensing.md`.

## Installer (for the store)
`installer\build.bat` builds the release package `installer\output\DonnaMedina-<version>.zip` (the installer plus `manifest.json`) from the Release build, with the store's `build.ps1` and Inno Setup recipe (needs Inno Setup 6.3+, e.g. `winget install JRSoftware.InnoSetup --scope user`). `installer\output\DonnaMedinaSetup.exe` is the installer alone, for testing. The version comes from `source\version.h`; raise it there for each release. The plugin's IDs come from the store's `installer\product.json`, and the build stops if `source\license.h` doesn't match it. Send the store owner the zip. Don't edit the store's files in `installer\` (`build.ps1`, `rogue-syntax.iss`, `eula.txt`, `product.json`, ...); replace them when the store sends a new pack. See `docs/07-licensing.md` section 6.
