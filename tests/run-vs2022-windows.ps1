$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$version = (Get-Content "$vs\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt").Trim()
$tools = "$vs\VC\Tools\MSVC\$version\bin\Hostx64\x64"
$out = Join-Path $root 'build\vs2022\runtime'
New-Item -ItemType Directory -Force $out | Out-Null
function Invoke-Native([string]$exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE" }
}
Invoke-Native "$tools\ml64.exe" @('/nologo', '/c', "/Fo$out\runtime.obj", "$PSScriptRoot\windows-runtime.asm")
$declarations = @()
foreach ($configuration in @('Debug', 'Release')) {
    $compiler = "$root\build\vs2022\$configuration\shalimarc.exe"
    foreach ($case in @(
        @{Name='sqrt'; Fixture='rev45_sqrt_literal.shl'; Export='verify_sqrt'},
        @{Name='integer'; Fixture='rev51_multi_caller.shl'; Export='caller'},
        @{Name='real'; Fixture='rev51_multi_real_caller.shl'; Export='caller_real'}
    )) {
        $name = "$configuration-$($case.Name)"
        Invoke-Native $compiler @("$PSScriptRoot\$($case.Fixture)", '--check')
        Invoke-Native $compiler @("$PSScriptRoot\$($case.Fixture)", '--object', "$out\$name.obj", "--$($configuration.ToLowerInvariant())")
        $linkArguments = @('/nologo','/dll','/noentry','/nodefaultlib','/machine:x64',"/export:$($case.Export)","/out:$out\$name.dll","$out\$name.obj")
        if ($case.Name -eq 'sqrt') { $linkArguments += "$out\runtime.obj" }
        Invoke-Native "$tools\link.exe" $linkArguments
        $type = "$configuration$($case.Name)"
        $returnType = if ($case.Name -eq 'real') { 'double' } else { 'int' }
        $dll = (Join-Path $out "$name.dll").Replace('\','\\')
        $declarations += "public static class $type { [System.Runtime.InteropServices.DllImport(`"$dll`", EntryPoint=`"$($case.Export)`", CallingConvention=System.Runtime.InteropServices.CallingConvention.Cdecl)] public static extern $returnType Run(); }"
    }
}
Add-Type -TypeDefinition ($declarations -join "`n")
$results = @(
    @{Case='Debug sqrt'; Actual=[Debugsqrt]::Run(); Expected=1},
    @{Case='Release sqrt'; Actual=[Releasesqrt]::Run(); Expected=1},
    @{Case='Debug integer multi-return'; Actual=[Debuginteger]::Run(); Expected=12},
    @{Case='Release integer multi-return'; Actual=[Releaseinteger]::Run(); Expected=12},
    @{Case='Debug real multi-return'; Actual=[Debugreal]::Run(); Expected=3.5},
    @{Case='Release real multi-return'; Actual=[Releasereal]::Run(); Expected=3.5}
)
foreach ($result in $results) {
    if ($result.Actual -ne $result.Expected) { throw "$($result.Case): expected $($result.Expected), got $($result.Actual)" }
    Write-Host "PASS $($result.Case): $($result.Actual)"
}
$results | ConvertTo-Json | Set-Content "$out\results.json"
