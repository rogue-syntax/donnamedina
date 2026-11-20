# Installer recipe

One Inno Setup script for every Rogue Syntax plugin: `rogue-syntax.iss`.
Your plugin adds one small file in `plugins/` with its identity.

**Set up once:** copy `plugins\example.iss` to `plugins\<PluginKey>.iss` and
fill in your plugin's name, registry subkey (`PluginKey`), Installer ID
(`AppGuid`) and `.vst3` file name. They come from the plugin's name (spec 1.7).
They never change after release.

**Build (on Windows, Inno Setup 6.3+):**
1. Put the plugin's `.vst3` in `installer\in\`.
2. `iscc plugins\<PluginKey>.iss /DPluginVersion=1.0.0`
3. Result: `installer\output\<PluginKey>Setup.exe`. Send it, with its
   version number, to the store owner.

**Licence agreement:** `eula.txt` is shown by every installer; the customer
must accept it. Don't change it; the store owner updates it.

**Sign it** (once Rogue Syntax has a certificate): add
`/Ssigntool="signtool sign /fd sha256 /tr <timestamp url> /td sha256 $f" /DSignTool=signtool`.

**Tested** automatically on a Windows machine at GitHub whenever this folder
changes (`.github/workflows/installer-test.yml`).
