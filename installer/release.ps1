# Donna Medina release package: installer\output\DonnaMedina-<version>.zip (the installer plus
# manifest.json), built by the store's build.ps1 (README.md; spec docs/store/licensing-v1.md 1.9).
# Build the plugin first (Release|x64, BUILDING.md). Run it through build.bat, or:
#   powershell -ExecutionPolicy Bypass -File installer\release.ps1 [-Iscc <path to ISCC.exe>]
# What this adds to build.ps1:
#   - the version comes from source\version.h (the plugin's own version, which the store requires);
#   - the plugin's identity (source\license.h) must equal the store's (product.json): the store
#     checks the manifest against product.json only, so it can't see a plugin built with other IDs;
#   - the plugin is installed as DonnaMedinaVST64.vst3 (the name it has always had);
#   - ISCC.exe is also looked for in a per-user Inno Setup install;
#   - it refuses a recording build (record-for-video, source\videoRecord.h).
param([string]$Iscc = "")
$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$proj = Split-Path -Parent $here

# ---- version (source\version.h)
$vh = Get-Content -Raw -LiteralPath (Join-Path $proj "source\version.h")
function Def([string]$n) {
  if ($vh -match ("#define\s+" + $n + "\s+""(\d+)""")) { return $matches[1] }
  throw "no $n in source\version.h"
}
$version = (Def "MAJOR_VERSION_STR") + "." + (Def "SUB_VERSION_STR") + "." + (Def "RELEASE_NUMBER_STR")

# ---- identity: source\license.h against product.json
$brief = Join-Path $here "product.json"
if (-not (Test-Path -LiteralPath $brief)) { throw "No installer\product.json: it comes in the store's developer pack." }
$b = Get-Content -Raw -Encoding UTF8 -LiteralPath $brief | ConvertFrom-Json
$lh = Get-Content -Raw -LiteralPath (Join-Path $proj "source\license.h")
function Const([string]$n) {
  if ($lh -match ($n + "\s*=\s*L?""([^""]*)""")) { return $matches[1] }
  throw "no $n in source\license.h"
}
# kRegSubKey is a C string, so its backslashes are doubled in the file
$pairs = @(
  @("product_id", $b.product_id, (Const "kProduct")),
  @("registry_key", ("Software\\Rogue-Syntax\\Registrations\\" + $b.registry_key), (Const "kRegSubKey")),
  @("shop_page", $b.shop_page, (Const "kShopUrl")))
foreach ($p in $pairs) {
  if ($p[1] -cne $p[2]) { throw "source\license.h doesn't match product.json ($($p[0])): '$($p[2])' vs '$($p[1])'" }
}

# ---- the plugin, under its installed name
$vst = Join-Path $proj "x64\Release\RSSynth001donnaMedina.vst3"
if (-not (Test-Path -LiteralPath $vst)) { throw "$vst not found: build Release|x64 first." }
$in = Join-Path $here "in"
New-Item -ItemType Directory -Force $in | Out-Null
$plugin = Join-Path $in "DonnaMedinaVST64.vst3"

# ---- never a recording build (record-for-video, source\videoRecord.h, docs\08-video.md)
$vr = Get-Content -Raw -LiteralPath (Join-Path $proj "source\videoRecord.h")
if ($vr -match '(?m)^[ \t]*#[ \t]*define[ \t]+RS_VIDEO_RECORD\b') {
  throw "source\videoRecord.h has RS_VIDEO_RECORD switched on: comment the #define out and rebuild Release|x64."
}
$bytes = [IO.File]::ReadAllBytes($vst)
if ([Text.Encoding]::GetEncoding(28591).GetString($bytes).Contains("DONNA-MEDINA-VIDEO-RECORD-BUILD")) {
  throw "$vst is a recording build (record-for-video): rebuild Release|x64 with RS_VIDEO_RECORD commented out."
}
Copy-Item -LiteralPath $vst -Destination $plugin -Force

# ---- Inno Setup
if (-not $Iscc) {
  $cands = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe")
  $Iscc = $cands | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
  if (-not $Iscc) { $c = Get-Command iscc -ErrorAction SilentlyContinue; if ($c) { $Iscc = $c.Source } }
  if (-not $Iscc) { throw "ISCC.exe not found: install Inno Setup 6.3 or newer, or give -Iscc." }
}

# the manifest records the commit; say so if the working copy has changes it doesn't include
$dirty = & git -C $proj status --porcelain --untracked-files=no 2>$null
if ($dirty) { Write-Warning "uncommitted changes: the manifest's git_commit won't describe this build exactly" }

& (Join-Path $here "build.ps1") -Version $version -Plugin $plugin -Iscc $Iscc
