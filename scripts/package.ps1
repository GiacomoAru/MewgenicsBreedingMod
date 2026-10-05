# Builds the release DLL (CB_DEV_TOOLS=OFF) and zips it into outputs\UnnaturalSelection-<version>.zip.
# The zip holds one folder, UnnaturalSelection\, with the mod files plus the license files (MIT requires them
# to travel with the DLL).
#
#   powershell -ExecutionPolicy Bypass -File scripts\package.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$pkg = Join-Path $root "mod\UnnaturalSelection"

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Release
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# mod\UnnaturalSelection holds exactly these three files.
$expected = @("unnatural_selection.dll", "config.ini", "description.json")
$actual = (Get-ChildItem $pkg -File).Name | Sort-Object
if (Compare-Object ($expected | Sort-Object) $actual) { throw "Unexpected files in ${pkg}: $($actual -join ', ')" }

$version = (Get-Content (Join-Path $pkg "description.json") -Raw | ConvertFrom-Json).version
$outDir = Join-Path $root "outputs"
$stage = Join-Path $outDir "package-stage"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
$folder = Join-Path $stage "UnnaturalSelection"
New-Item -ItemType Directory -Force $folder | Out-Null
Copy-Item (Join-Path $pkg "*") $folder
foreach ($f in "LICENSE.md", "ATTRIBUTION.md", "README.md") { Copy-Item (Join-Path $root $f) $folder }

$zip = Join-Path $outDir "UnnaturalSelection-$version.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $folder -DestinationPath $zip
Remove-Item $stage -Recurse -Force
Write-Host "Packaged $zip"
