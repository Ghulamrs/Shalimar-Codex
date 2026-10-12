# TMS6747 / CCS 5.5 simulator assessment — 11 October 2026

**Simulator readiness: PASS. Shalimar-generated TMS6747 executable verification: BLOCKED by missing target integration.**

## Installed and exercised tools

- CCS 5.5 installation: `C:\ti\ccsv5`.
- TI C6000 compiler/linker 7.4.4: `C:\ti\ccsv5\tools\compiler\c6000_7.4.4\bin\cl6x.exe`.
- Debug Server Scripting launcher: `C:\ti\ccsv5\ccs_base\scripting\bin\dss.bat`.
- Actual selected simulator: **C6747 Device Functional Simulator, Little Endian**, installed configuration `tisim_c6747_pv.xml`.

The older TI binaries did not launch successfully in the sandbox. After approved execution outside the sandbox, the compiler and Debug Server ran successfully. No installation changes were needed. CCS 7 is also present, but was not required for this simulator check.

## Executed control test

Built `tests/tms6747-simulator/control.c` with `-mv6740 --abi=eabi -g`, linked with `rts6740_elf.lib` and the supplied local linker command file. The resulting DSP executable is `build/tms6747/control.out`, not a Windows PE executable. The map records entry point `_c_int00` at `0x11801680` and L2RAM at `0x11800000`.

The DSS script selects the actual C6747 device simulator, connects, loads the executable, sets a breakpoint at `halt_here`, runs it, and evaluates `probe_result`. The source computes 17 + 25 through volatile inputs.

Observed output:

```text
CONNECTED C6747 Device Functional Simulator
TI CONTROL RESULT=42 EXPECTED=42
PASS simulator control; this is NOT Shalimar-generated code
```

The DSS process returned exit code 0. This demonstrates simulator connection, target executable loading, execution and result inspection. The control is TI-compiled C, so it provides **no Shalimar code-generation conformance evidence**.

## Shalimar target probe and source inspection

Invoked the current Release compiler on the small `rev54_codeview_probe.shl` fixture with `--object ... --target=tms6747`. It returned exit code 2 and:

```text
Unknown argument: --target=tms6747
```

The active `src/main.cpp` uses the x86-64 selector/encoder and Windows COFF writer directly. It does not expose target selection or an integrated TMS6747 emission pipeline. `src/backend/tms6747/TargetContract.cpp` labels the object/debug formats unverified; its header explicitly says it declares the target without claiming instruction/object support. `src/backend/legacy_port/Tms6747.cpp` contains an assembly-text emitter, but is not connected to the Shalimar typed IR driver. Its successful compilation is not successful DSP code generation.

Consequently there is currently no Shalimar-generated DSP object or executable to load into this working simulator. The Windows `shalimarc.exe` is the host compiler, and its Windows x64 generated objects/DLLs are not C6747 target programs.

## Reproduce the simulator control

From the workspace compiler directory in PowerShell, using a process able to launch the older TI tools:

```powershell
New-Item -ItemType Directory -Force build/tms6747 | Out-Null
& C:/ti/ccsv5/tools/compiler/c6000_7.4.4/bin/cl6x.exe -mv6740 --abi=eabi -g --include_path=C:/ti/ccsv5/tools/compiler/c6000_7.4.4/include --obj_directory=build/tms6747 tests/tms6747-simulator/control.c --run_linker --search_path=C:/ti/ccsv5/tools/compiler/c6000_7.4.4/lib --rom_model --output_file=build/tms6747/control.out --map_file=build/tms6747/control.map tests/tms6747-simulator/control.cmd --library=rts6740_elf.lib
& C:/ti/ccsv5/ccs_base/scripting/bin/dss.bat C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/ShalimarCompiler/tests/tms6747-simulator/run-control.js
```

The local DSS script contains this workspace's absolute path. It has a 20-second scripting-operation timeout and checks the numerical result before declaring the control passed.

## Next prerequisite for Shalimar verification

Connect a genuine C6747 backend to typed Shalimar IR and target selection, emit valid TI-assembler input or native target objects, implement the target ABI and required runtime paths, and link a DSP `.out` executable. TI's assembler/linker may package Shalimar-generated assembly without substituting a C backend. That executable can then be loaded by the demonstrated DSS simulator harness and compared with independently compiled TI C/C++ oracle cases. Begin with scalar arithmetic/calls, then add control flow, arrays, multi-return and the normative example corpus.

No compiler-source implementation was changed during this assessment; full DSP backend work remains outstanding.
