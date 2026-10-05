# Builds clean_breeding.dll (RelWithDebInfo) and copies it to mod\CleanBreeding\.
#
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1
#
# Needs Visual Studio 2022 Build Tools with "Desktop development with C++" (ships CMake).

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$src = Join-Path $root "src"
$build = Join-Path $root "build"

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
Write-Host "Using $cmake"

& $cmake -S $src -B $build -A x64
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $cmake --build $build --config RelWithDebInfo --parallel
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# Unit checks (Debug, so assert() is active)
foreach ($t in "test_config", "test_breed_logic") {
    & $cmake --build $build --config Debug --target $t
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    & "$build\Debug\$t.exe"
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}

$out = Join-Path $root "mod\CleanBreeding"
New-Item -ItemType Directory -Force $out | Out-Null
Copy-Item "$build\clean_breeding\RelWithDebInfo\clean_breeding.dll" $out -Force
Write-Host "Built $out\clean_breeding.dll"
