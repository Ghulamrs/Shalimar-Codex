# rev47 — Windows native runtime acceptance gate

This revision introduces a **numerical runtime test harness** for the existing S-native `sqrt(81.)` code path. An independently assembled Windows x64 `sqrt` shim uses the processor's `sqrtsd` instruction; a second assembly function `verify_sqrt` calls Shalimar's exported `root`, compares the returned XMM0 value to 9.0, and returns 1 on success or 0 on failure. The resulting DLL exports `verify_sqrt` for a Windows-host test runner.

`tests/run-rev47-windows-runtime.sh` builds and validates Debug and Release DLLs, and checks that the export exists. **This Linux build environment does not execute the Windows DLL.** No runtime correctness claim is made until `verify_sqrt` is actually invoked on a Windows host and returns 1 in both modes.

## Windows acceptance procedure

Build the DLL on Windows or transfer the built DLL from the test script's temporary output by adapting its output directory. Then invoke `verify_sqrt` through a Windows x64 test executable or PowerShell P/Invoke, and assert the result is 1 for both Debug and Release. This is a controlled native test shim, not a production Windows CRT import or general-purpose math library. Native CodeView/PDB support remains pending.

## Reproduction

`tests/run-rev47-windows-runtime.sh /path/to/shalimarc`

Requires clang, lld-link, file and objdump. The harness does not invoke C-mode compilation.

The archive also includes prelinked `tests/shalimar-debug.dll` and `tests/shalimar-release.dll` for Windows x64, plus `tests/run-rev47-on-windows.ps1`. Run the PowerShell script from its own directory using Windows x64 PowerShell (or a compatible 64-bit PowerShell runtime). DLL execution has **not** been performed in this build environment.
