# Installer recipe

One Inno Setup script for every Rogue Syntax plugin: `rogue-syntax.iss`.
`build.ps1` builds your plugin's installer with it and packs the release
for the store (spec 1.9).

**You need:** Windows, Inno Setup 6.3+, and `product.json` from the store
owner. It's in your plugin's developer pack and holds your plugin's identity.
Put it next to `build.ps1`. Don't edit it.

**Build a release:**

    .\build.ps1 -Version 1.0.1 -Plugin C:\path\to\YourPlugin.vst3

`-Plugin` is your `.vst3` file or bundle folder. The version must be newer
than `newer_than` in `product.json`. The result is
`output\<PluginKey>-<version>.zip`: the installer plus `manifest.json`.
Send that zip to the store owner. The installer alone is
`output\<PluginKey>Setup.exe`, for testing.

**Licence agreement:** `eula.txt` is shown by every installer; the customer
must accept it. Don't change it; the store owner updates it.

**Pack version:** `PACK_VERSION`. When the store owner sends a new pack, build
with it; the store refuses packages built with an outdated recipe or EULA.

**Sign it** (once Rogue Syntax has a certificate): the store owner will
update the recipe.

**Tested** automatically whenever this folder changes
(`.github/workflows/installer-test.yml`): it builds `example-product.json`'s
package on Windows, installs it, and runs the store's upload checks on it.
