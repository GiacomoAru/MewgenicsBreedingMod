# Builds clean_breeding.dll.
#
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1            # development build (CB_DEV_TOOLS=ON)
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Release   # release build (CB_DEV_TOOLS=OFF)
#
# Development: build\ -> outputs\dev\clean_breeding.dll (simulator, test suite, snapshots, Debug menu, [debug] helpers).
# Release:     build-release\ -> mod\CleanBreeding\clean_breeding.dll (what package.ps1 zips).
#
# Needs Visual Studio 2022 Build Tools with "Desktop development with C++" (ships CMake).

param([switch]$Release)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$src = Join-Path $root "src"
$build = Join-Path $root ($(if ($Release) { "build-release" } else { "build" }))
$devTools = if ($Release) { "OFF" } else { "ON" }

function Find-CMake {
    $onPath = Get-Command cmake -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }
    $roots = @($env:ProgramFiles, ${env:ProgramFiles(x86)}) | Where-Object { $_ }
    foreach ($r in $roots) {
        $found = Get-ChildItem "$r\Microsoft Visual Studio\*\*\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending | Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    throw "CMake not found. Install Visual Studio Build Tools with 'Desktop development with C++'."
}

$cmake = Find-CMake
Write-Host "Using $cmake (CB_DEV_TOOLS=$devTools)"

& $cmake -S $src -B $build -A x64 "-DCB_DEV_TOOLS=$devTools"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $cmake --build $build --config RelWithDebInfo --parallel --target clean_breeding
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# Unit checks (Debug, so assert() is active)
foreach ($t in "test_config", "test_breed_logic") {
    & $cmake --build $build --config Debug --target $t
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    & "$build\Debug\$t.exe"
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}

$out = if ($Release) { Join-Path $root "mod\CleanBreeding" } else { Join-Path $root "outputs\dev" }
New-Item -ItemType Directory -Force $out | Out-Null
Copy-Item "$build\clean_breeding\RelWithDebInfo\clean_breeding.dll" $out -Force
Write-Host "Built $out\clean_breeding.dll"
