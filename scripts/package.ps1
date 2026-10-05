# Builds the release DLL (CB_DEV_TOOLS=OFF) and zips mod\CleanBreeding\ into outputs\CleanBreeding-<version>.zip.
#
#   powershell -ExecutionPolicy Bypass -File scripts\package.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$pkg = Join-Path $root "mod\CleanBreeding"

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Release
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# The package holds exactly these three files.
$expected = @("clean_breeding.dll", "config.ini", "description.json")
$actual = (Get-ChildItem $pkg -File).Name | Sort-Object
if (Compare-Object ($expected | Sort-Object) $actual) { throw "Unexpected files in ${pkg}: $($actual -join ', ')" }

$version = (Get-Content (Join-Path $pkg "description.json") -Raw | ConvertFrom-Json).version
$outDir = Join-Path $root "outputs"
New-Item -ItemType Directory -Force $outDir | Out-Null
$zip = Join-Path $outDir "CleanBreeding-$version.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $pkg -DestinationPath $zip
Write-Host "Packaged $zip"
