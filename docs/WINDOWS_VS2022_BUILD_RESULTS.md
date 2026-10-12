# Windows Visual Studio 2022 build — 11 October 2026

Source inspected: `C:\Users\GRA\source\ShalimarCompiler`.
Build copy: `C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\ShalimarCompiler`.
The original source directory was left unchanged. The separately mentioned Rev54 PARTIAL ZIP was not overlaid onto this source.

The supplied root Visual Studio project is stale: it references removed CodeGenerator.cpp, Optimizer.cpp and IRCodeGenerator.cpp files and omits the current native backend. The current CMake source list was used to generate a working Visual Studio solution at `build/vs2022/ShalimarCompiler.sln`.

## Environment and result

- Visual Studio Community 2022 17.14.41; v143 tools.
- MSVC compiler 19.44.35229.0, x64 host and target.
- Windows SDK 10.0.26100.0.
- Debug and Release compiler builds: PASS.
- Executables: `build/vs2022/Debug/shalimarc.exe` and `build/vs2022/Release/shalimarc.exe`.
- Logs: `build/vs2022/build-Debug.log` and `build/vs2022/build-Release.log`.

## Native runtime validation

`tests/run-vs2022-windows.ps1` checks each fixture, generates fresh Windows x64 COFF objects with both compiler configurations and corresponding compiler modes, links DLLs using Microsoft link.exe, and calls their exports in the Windows host through P/Invoke.

| Fixture | Debug | Release |
| --- | --- | --- |
| rev45_sqrt_literal.shl, assembly verifier of sqrt(81) = 9 | PASS (1) | PASS (1) |
| rev51_multi_caller.shl, integer result | PASS (12) | PASS (12) |
| rev51_multi_real_caller.shl, real result | PASS (3.5) | PASS (3.5) |

The sqrt import is satisfied by the independent MASM `sqrtsd` test shim, not a production CRT import. DLLs are linked without a CRT entry point. These tests establish execution of these restricted examples, not complete standalone Shalimar program or language support. Machine-readable results are in `build/vs2022/runtime/results.json`.

## Reproduce

From the build copy in PowerShell:

```powershell
.\build-windows.ps1
.\tests\run-vs2022-windows.ps1
.\build\vs2022\Release\shalimarc.exe .\tests\rev51_multi_caller.shl --check
```

Open `build/vs2022/ShalimarCompiler.sln` in Visual Studio 2022 and select Debug or Release with platform x64. Set shalimarc as the startup project and supply input filename and compiler flags under Debugging / Command Arguments.

## Limits

Build warnings remain: sprintf deprecation (C4996), double-to-long narrowing (C4244), constant conditions (C4127), and Release unreachable code (C4702). No compiler source changes were needed for this build. Source inspection and development history establish that the active compiler is S-native with a restricted Windows x64 selector; older README sections describe a removed C backend. Native generated-code CodeView/PDB support and complete ABI/language coverage remain incomplete. The compiler itself has Debug build symbols; this does not imply source debugging of generated Shalimar programs. The historical shell regression suite was not run in full.
