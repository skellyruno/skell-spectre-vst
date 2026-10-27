param(
    [string]$Generator = "",
    [string]$Configuration = "Release",
    [string]$JucePath = ""
)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$configureArgs = @('-S', '.', '-B', 'build', '-A', 'x64', '-DSKELL_BUILD_TESTS=ON')
if ($Generator) { $configureArgs += @('-G', $Generator) }
if ($JucePath) { $configureArgs += "-DJUCE_PATH=$JucePath" }
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed. Check Visual Studio, JUCE_PATH and network access.' }
& cmake --build build --config $Configuration --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'SKELL SPECTRE build failed.' }
& ctest --test-dir build -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'DSP regression tests failed.' }
Write-Host 'Build completed: build/SkellSpectre_artefacts/Release/VST3 (plus Standalone).'
