# Vendored dependencies

Everything the plugin needs outside the VST3 SDK lives here, so the project builds without `C:\D\...` or other machine-specific folders.

| Folder | What | Origin | Notes |
|---|---|---|---|
| `glew/include/GL/glew.h`, `wglew.h` | GLEW headers | copied 2026-11-12 from `C:\D\SFML-2.5.1-64\include\SFML\gl\` | used with `GLEW_STATIC` |
| `glew/lib/x64/glew32s.lib` | GLEW static lib, x64 | copied from `C:\D\SFML-2.5.1-64\lib\` | |
| `libbcrypt/` | trusch/libbcrypt sources (bcrypt.c, crypt_blowfish.c, crypt_gensalt.c, wrapper.c + headers) | copied from the project's `libbcrypt/` clone (commit d6523c3), identical to `VST3_SDK\build\libbcrypt` | compiled straight into the plugin; replaces the prebuilt `VST3_SDK\build\libbcrypt\build\RelWithDebInfo\bcrypt.lib`. `x86.S` is not used on MSVC. |

Not here on purpose: **revent** is shared with the modded VSTGUI (`vstgui4/vstgui/lib/controls/cbuttons.h` includes it), so the single copy lives at `C:\vst3sdk\VST3_SDK\external\revent` (SDK level, outside this repo). It was copied from `C:\D\SFML-2.5.1-64\include\SFML\revent` (the newest of the three copies that existed).

Removed from the link (not used by live code): `sfml-system-s`, `sfml-window-s`, `glfw3`, `freetype`, `winmm`. The `C:\D\bcrypt` headers were a damaged copy (`std::` find-replaced to `stC::` in the non-Windows branch) and are not used.
