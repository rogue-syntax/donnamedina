# Builds a release package for the Rogue Syntax store (docs/licensing-v1.md 1.9):
#   output\<PluginKey>-<version>.zip  =  <PluginKey>Setup.exe + manifest.json
#
#   .\build.ps1 -Version 1.0.1 -Plugin C:\path\to\YourPlugin.vst3
#
# Needs Inno Setup 6.3+ and product.json from the store owner next to this
# script (or -Brief path). -Plugin is the .vst3 file or bundle folder.
# -Commit: your git commit (found automatically if -Plugin is in a git repo).
param(
  [Parameter(Mandatory = $true)][string]$Version,
  [Parameter(Mandatory = $true)][string]$Plugin,
  [string]$Brief = (Join-Path $PSScriptRoot "product.json"),
  [string]$Commit = "",
  [string]$Iscc = ""
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2
$here = $PSScriptRoot

function Sha256Bytes([byte[]]$b) {
  $h = [Security.Cryptography.SHA256]::Create()
  ($h.ComputeHash($b) | ForEach-Object { $_.ToString("x2") }) -join ""
}
function FileSha([string]$p) { (Get-FileHash -Algorithm SHA256 -LiteralPath $p).Hash.ToLower() }
# text files: CRLF counts as LF, so any checkout gives the same value
function TextSha([string]$p) {
  $l1 = [Text.Encoding]::GetEncoding(28591)
  $s = $l1.GetString([IO.File]::ReadAllBytes($p)).Replace("`r`n", "`n")
  Sha256Bytes $l1.GetBytes($s)
}
# a .vst3 file: its SHA-256; a bundle folder: SHA-256 of "<path> <sha256>`n" per file, sorted
function PluginSha([string]$p) {
  if (-not (Test-Path -LiteralPath $p -PathType Container)) { return FileSha $p }
  $root = (Resolve-Path -LiteralPath $p).Path.TrimEnd('\')
  [string[]]$lines = @(Get-ChildItem -LiteralPath $root -Recurse -File -Force | ForEach-Object {
      $_.FullName.Substring($root.Length + 1).Replace('\', '/') + " " + (FileSha $_.FullName) + "`n" })
  [Array]::Sort($lines, [StringComparer]::Ordinal)
  Sha256Bytes ([Text.Encoding]::UTF8.GetBytes(($lines -join "")))
}
function VersionNewer([string]$a, [string]$b) {
  $pa = $a.Split('.'); $pb = $b.Split('.')
  for ($i = 0; $i -lt [Math]::Max($pa.Length, $pb.Length); $i++) {
    $x = 0; $y = 0
    if ($i -lt $pa.Length) { $x = [int]$pa[$i] }
    if ($i -lt $pb.Length) { $y = [int]$pb[$i] }
    if ($x -ne $y) { return $x -gt $y }
  }
  return $false
}

# ---- inputs
if ($Version -notmatch '^\d{1,4}(\.\d{1,4}){0,3}$') { throw "Version looks like 1.0.2" }
if (-not (Test-Path -LiteralPath $Brief)) { throw "No product.json at $Brief. Get it from the store owner (it's in your plugin's developer pack)." }
$b = Get-Content -Raw -Encoding UTF8 -LiteralPath $Brief | ConvertFrom-Json
if ($b.brief -ne 1) { throw "product.json isn't a store brief (brief: 1)" }
foreach ($k in "name", "product_id", "registry_key", "licence_prefix", "shop_page", "app_guid") {
  if (-not $b.$k) { throw "product.json has no $k" }
}
if ($b.name -match '"') { throw "the plugin name can't contain a double quote" }
if ($b.newer_than -and -not (VersionNewer $Version $b.newer_than)) {
  throw "Version $Version must be newer than the published $($b.newer_than)"
}
if (-not (Test-Path -LiteralPath $Plugin)) { throw "No plugin at $Plugin" }
$pluginPath = (Resolve-Path -LiteralPath $Plugin).Path.TrimEnd('\')
$pluginFile = Split-Path -Leaf $pluginPath
if ($pluginFile -notmatch '\.vst3$') { throw "-Plugin must be a .vst3 file or bundle" }
$pack = (Get-Content -Raw -LiteralPath (Join-Path $here "PACK_VERSION")).Trim()
if ($b.pack_version -and $b.pack_version -ne $pack) {
  Write-Warning "product.json was made for pack $($b.pack_version), this is pack $pack. Ask the store owner for a fresh developer pack if the upload is refused."
}

# ---- Inno Setup
if (-not $Iscc) {
  $cands = @("${env:ProgramFiles(x86)}\Inno Setup 7\ISCC.exe", "$env:ProgramFiles\Inno Setup 7\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe")
  $Iscc = $cands | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
  if (-not $Iscc) { $c = Get-Command iscc -ErrorAction SilentlyContinue; if ($c) { $Iscc = $c.Source } }
  if (-not $Iscc) { throw "Inno Setup's ISCC.exe not found; give -Iscc path" }
}

# ---- build the installer
$key = $b.registry_key
$buildDir = Join-Path $here "build"
$outDir = Join-Path $here "output"
New-Item -ItemType Directory -Force $buildDir, $outDir | Out-Null
$iss = Join-Path $buildDir "$key.iss"
$text = "; made by build.ps1; don't edit`r`n" +
"#define PluginName `"$($b.name)`"`r`n" +
"#define PluginKey `"$key`"`r`n" +
"#define AppGuid `"$($b.app_guid.ToUpper())`"`r`n" +
"#define PluginVersion `"$Version`"`r`n" +
"#define Vst3Source `"$pluginPath`"`r`n" +
"#include `"..\rogue-syntax.iss`"`r`n"
[IO.File]::WriteAllText($iss, $text, (New-Object Text.UTF8Encoding $true))
$exe = Join-Path $outDir "${key}Setup.exe"
if (Test-Path -LiteralPath $exe) { Remove-Item -LiteralPath $exe }
& $Iscc "/Qp" $iss
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed (exit $LASTEXITCODE)" }

# ---- manifest and zip
if (-not $Commit) {
  try {
    $d = $pluginPath; if (-not (Test-Path -LiteralPath $d -PathType Container)) { $d = Split-Path $d }
    $Commit = (& git -C $d rev-parse HEAD 2>$null)
    if ($LASTEXITCODE -ne 0) { $Commit = "" }
  } catch { $Commit = "" }
}
$m = [ordered]@{
  manifest         = 1
  name             = $b.name
  product_id       = $b.product_id
  registry_key     = $key
  licence_prefix   = $b.licence_prefix
  shop_page        = $b.shop_page
  app_guid         = $b.app_guid.ToUpper()
  version          = $Version
  installer_file   = "${key}Setup.exe"
  installer_sha256 = FileSha $exe
  plugin_file      = $pluginFile
  plugin_sha256    = PluginSha $pluginPath
  pack_version     = $pack
  recipe_sha256    = TextSha (Join-Path $here "rogue-syntax.iss")
  eula_sha256      = TextSha (Join-Path $here "eula.txt")
  git_commit       = "$Commit".Trim()
  built            = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
}
$man = Join-Path $buildDir "manifest.json"
[IO.File]::WriteAllText($man, ($m | ConvertTo-Json), (New-Object Text.UTF8Encoding $false))
$zip = Join-Path $outDir "$key-$Version.zip"
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip }
Compress-Archive -LiteralPath $exe, $man -DestinationPath $zip
Write-Host ""
Write-Host "Release package: $zip"
Write-Host "Send it to the store owner. They upload it in the dashboard (Products)."
