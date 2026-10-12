param([ValidateSet('Debug','Release','Both')][string]$Configuration = 'Both')
$ErrorActionPreference = 'Stop'
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio 2022 C++ tools were not found.' }
$cmake = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$build = Join-Path $PSScriptRoot 'build\vs2022'
& $cmake -S $PSScriptRoot -B $build -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
$configurations = if ($Configuration -eq 'Both') { @('Debug','Release') } else { @($Configuration) }
foreach ($mode in $configurations) {
    & $cmake --build $build --config $mode --parallel 4
    if ($LASTEXITCODE -ne 0) { throw "$mode build failed." }
}
