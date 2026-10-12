param(
    [Parameter(Mandatory=$true)][string]$ShalimarCompiler,
    [string]$TiRoot = 'C:\ti\ccsv5\tools\compiler\c6000_7.4.4',
    [string]$Dss = 'C:\ti\ccsv5\ccs_base\scripting\bin\dss.bat',
    [switch]$RunSimulator
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$outdir = Join-Path $project 'build\tms6747-shalimar'
New-Item -ItemType Directory -Force $outdir | Out-Null
$out = Join-Path $outdir 'shalimar_42.out'
& $ShalimarCompiler (Join-Path $project 'tests\tms6747\return_42.shl') --target=tms6747 --toolchain=ti --debug --dsp-entry answer --ti-root $TiRoot -o $out
if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $out)) { throw 'Integrated DSP build failed; see adjacent TI logs' }
Write-Host "Built DSP executable: $out"
if ($RunSimulator) {
    $result = & $Dss (Join-Path $PSScriptRoot 'run-shalimar.js') $project.Replace('\','/') $out.Replace('\','/') 42 int 2>&1
    $result | Write-Host
    if ($LASTEXITCODE -ne 0 -or ($result -join "`n") -notmatch 'PASS: Shalimar-generated DSP function' -or ($result -join "`n") -match 'FAIL') {
        throw 'CCS simulator verification failed'
    }
}
