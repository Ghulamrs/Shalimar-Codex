$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ShalimarDebug {
    [DllImport("shalimar-debug.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int verify_sqrt();
}
public static class ShalimarRelease {
    [DllImport("shalimar-release.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int verify_sqrt();
}
'@
if ([ShalimarDebug]::verify_sqrt() -ne 1) { throw 'Debug native sqrt returned incorrect value' }
if ([ShalimarRelease]::verify_sqrt() -ne 1) { throw 'Release native sqrt returned incorrect value' }
Write-Host 'rev47 Windows native Debug and Release runtime checks passed'
