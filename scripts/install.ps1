# Installs the mod into Mewtator's mods folder.
#
#   powershell -ExecutionPolicy Bypass -File scripts\install.ps1             # development build (outputs\dev) + configs\config.dev.ini
#   powershell -ExecutionPolicy Bypass -File scripts\install.ps1 -Release    # release package (mod\UnnaturalSelection)
#   ... [-ModsDir <path>]
#
# Default mods folder: the "Mewtator*\Mewtator\mods" folder inside this project.
# An existing config.ini in the mods folder is never overwritten (it holds the user's settings).
# The mod used to be called "Clean Breeding" (folder CleanBreeding): its config.ini is copied over, and the old folder is
# NOT deleted: remove it yourself, two DLLs of the same mod would hook breed twice.

param([switch]$Release, [string]$ModsDir)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
if (-not $ModsDir) {
    $m = Get-ChildItem $root -Directory -Filter "Mewtator*" | Select-Object -First 1
    if (-not $m) { throw "Mewtator folder not found in $root. Pass -ModsDir." }
    $ModsDir = Join-Path $m.FullName "Mewtator\mods"
}
if (-not (Test-Path $ModsDir)) { throw "Mods folder not found: $ModsDir" }

$dest = Join-Path $ModsDir "UnnaturalSelection"
$old = Join-Path $ModsDir "CleanBreeding"
New-Item -ItemType Directory -Force $dest | Out-Null
$pkg = Join-Path $root "mod\UnnaturalSelection"

if ($Release) {
    $files = Get-ChildItem $pkg
    $cfg = Join-Path $pkg "config.ini"
} else {
    $dll = Join-Path $root "outputs\dev\unnatural_selection.dll"
    if (-not (Test-Path $dll)) { throw "Development DLL not found: run scripts\build.ps1 first." }
    $files = @(Get-Item (Join-Path $pkg "description.json"), (Get-Item $dll))
    $cfg = Join-Path $root "configs\config.dev.ini"
}
$files | Where-Object { $_.Name -ne "config.ini" } | Copy-Item -Destination $dest -Force
if (-not (Test-Path "$dest\config.ini")) {
    if (Test-Path "$old\config.ini") {
        Copy-Item "$old\config.ini" "$dest\config.ini"   # keep the user's settings from the old folder
        Write-Host "Copied the settings from the old CleanBreeding folder."
    } else {
        Copy-Item $cfg "$dest\config.ini"
    }
}
Write-Host "Installed ($(if ($Release) { 'release' } else { 'development' })) to $dest"
if (Test-Path $old) {
    Write-Warning "The old folder still exists: $old"
    Write-Warning "Delete it (or disable it in Mewtator): two DLLs of the same mod would both hook breed."
}
