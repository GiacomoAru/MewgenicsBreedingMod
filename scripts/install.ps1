# Copies mod\CleanBreeding\ into Mewtator's mods folder.
#
#   powershell -ExecutionPolicy Bypass -File scripts\install.ps1 [-ModsDir <path>]
#
# Default: the "Mewtator*\Mewtator\mods" folder inside this project.

param([string]$ModsDir)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
if (-not $ModsDir) {
    $m = Get-ChildItem $root -Directory -Filter "Mewtator*" | Select-Object -First 1
    if (-not $m) { throw "Mewtator folder not found in $root. Pass -ModsDir." }
    $ModsDir = Join-Path $m.FullName "Mewtator\mods"
}
if (-not (Test-Path $ModsDir)) { throw "Mods folder not found: $ModsDir" }

$dest = Join-Path $ModsDir "CleanBreeding"
New-Item -ItemType Directory -Force $dest | Out-Null
# keep the user's config.ini if already installed
$keepCfg = Test-Path "$dest\config.ini"
Get-ChildItem "$root\mod\CleanBreeding" | Where-Object { -not ($keepCfg -and $_.Name -eq "config.ini") } |
    Copy-Item -Destination $dest -Force
Write-Host "Installed to $dest"
