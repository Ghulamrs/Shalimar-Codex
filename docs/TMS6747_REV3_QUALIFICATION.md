# TMS6747 Rev3 checkpoint — 11 October 2026

The compiler driver now builds real C6747 ELF executables. The user's source-based tool set is independently selectable and is the DSP default; TI tools remain a separate oracle/debugger path. Older assessment documents describe historical checkpoints and do not describe this revision.

## Independent tool paths

The user's executables remain at `C:\Users\GRA\source`: `asm6x.exe`, `lnk6x.exe`, `vm6747.exe`, and `sim6747.exe`. The matching runtime is `C:\Users\GRA\Developer\Claude\M10\RTS6x\build\rts6x.lib`. Their source projects remain unchanged under `C:\Users\GRA\Developer\Claude\M10`. A separate copy of the executables and runtime, with checksums and source locations, is in `toolsets/shalimar-c6747`; no TI binaries or TI runtime are included there.

The own-tool pipeline invokes ASM6x and LNK6x directly and links RTS6x. Its startup is generated assembly; no TI compiler is used. The TI pipeline uses `cl6x` to assemble emitted code, compile a small startup harness, and link its RTS. Shalimar operations execute as generated DSP instructions in both paths.

## Reproduce the verified user request

Run from the compiler project root:

```powershell
.\build\vs2022\Debug\shalimarc.exe tests\tms6747\return_42.shl --target=tms6747 --toolchain=shalimar --tools-root C:\Users\GRA\source --rts-lib C:\Users\GRA\Developer\Claude\M10\RTS6x\build\rts6x.lib --debug --dsp-entry answer -o build\own-tools-demo\answer.out
C:\Users\GRA\source\vm6747.exe build\own-tools-demo\answer.out.asm build\own-tools-demo\answer.out.startup.s -c
C:\Users\GRA\source\sim6747.exe --run build\own-tools-demo\answer.out --main-status -c
```

Both runs returned 42. VM6747 reported 96 cycles, 60 packets, zero native library calls; SIM6747 reported 130 cycles, 73 packets, `exit=C$$EXIT status=42`. These are the tools' reported counts, not a timing-conformance claim. The logs are `build/own-tools-demo/vm.log` and `sim.log`.

VM6747 interprets the generated assembly; it does not load ELF executables. SIM6747's `--run` decodes and executes the linked `.out`. CCS 5.5 independently ran the own-tool executable and also verified 42.

`SHALIMAR_C6747_TOOLS` can set the own-tool root. Both a flat directory and a `bin/lib` layout work. In the flat layout, put `rts6x.lib` beside the executables or use `--rts-lib`. `--toolchain=ti --ti-root PATH` explicitly selects the separate TI toolchain; `TI_C6000_ROOT` supplies its default root.

## Qualification evidence

- Both VS2022/MSVC Debug and Release host compiler builds succeed. The root solution's stale source list was repaired and both configurations build; CMake remains supported. The source continues to use C++98 constructs, but MSVC's C++14 compatibility mode is not an independent strict-C++98 proof.
- Twelve own-tool runtime fixtures cross two host builds and two source modes. `tests/tms6747-simulator/qualify-own.py` records build, assembly VM, machine simulator, and CCS checks in `build/tms6747-own/results.json`. VM checks compare exact integer or both binary64 result words; CCS reads the stored result. SIM6747 checks integer status and termination; its real-result status is zero, with exact real values checked by VM and CCS. SIM6747 masks the process status to eight bits; the harness also verifies the full status in its cycle log.
- The TI path passed 64 runtime/debugger cases before the independent own-tool driver addition: 48 runtime executions and 16 source debugger scenarios. Evidence is `build/tms6747-rev3/qualification-results.json`. Genuine DWARF resolves source breakpoints, scalar parameter/local types and locations, stack frames through `_c_int00`, source step-out, and continuation in Debug and optimized Release. The later own-tool addition changes tool selection and normalizes SUBDP's first operand onto the destination register side; this earlier 64-case record is not represented as a final rerun of that addition.
- Both assemblers separately accept 21 of all 167 `.shl/.shm` fixtures in each of the four host-build/source-mode combinations: 84 emitted objects per assembler, with no assembler failures among accepted programs. None of the 12 original normative examples is emitted by the restricted DSP selector. See `build/tms6747-corpus/results.json` and `build/tms6747-own-corpus/results.json`.
- TI driver contract checks pass 32/32, covering output modes, paths with spaces, missing tools, invalid entries and argument errors. Windows regression results are 145/167 front-end/IR stage completions, 27 native fixtures per combination, 108/108 DLL links and executions, 140 explicit result assertions, and 26/26 C++ tests. Stage completion is not a language-conformance verdict.

The tested DSP subset covers scalar int/real parameters and returns, constants and initialized locals, assignment, integer arithmetic/comparisons, real arithmetic, integer widening, branches, while loops, nested calls, recursion, and ten integer or real register arguments.

## Repairs

Connected typed IR to the existing TMS6747 emitter; added direct, quoted Windows subprocess launches, diagnostics and timeouts; executable/object output, startup and linker layouts; DWARF lines/types/locations and stack-pointer-based frame rules; and independent own-tool selection. Fixed Release block renumbering and false loop detection on backward-numbered nested-if joins. Natural back edges now require dominance. Added an optimizer regression. Corrected the Windows real-subtraction selector that had incorrectly selected addition. Normalized source paths for CCS breakpoints and normalized SUBDP operands for ASM6x. CCS 5.5 miscompares decimal real literals in debugger expressions; tests use exact integer scaling plus raw binary64 words instead.

## Qualification limits

**This is a working, tested scalar DSP subset, not a fully language-conforming compiler.** Arrays, references, globals, strings/chars, multiple outputs, print/library operations, for-loop operations, unary operations, division/remainder and powers remain unsupported by this DSP bridge and are rejected. Real-to-int conversion and general real comparisons are unsupported. Runtime integer overflow, uninitialized local semantics, recursion ceilings and stack exhaustion are not enforced/qualified. Generated code is deliberately serial with conservative NOP delays; VLIW scheduling and timing parity are not qualified. Other target backends were preserved but not requalified.

**Own-tool source debugging remains incomplete.** The supplied ASM6x rejects DWARF directives; VM6747 also rejects them. Own-tool `--debug` means unoptimized IR without DWARF. An explicit `--debug-symbols` request fails rather than silently falling back to TI. Actual source debugging is verified only on the explicitly selected TI path. A source index alone is not claimed to satisfy debugger integration.

## Rebuild after extraction

Existing CMake caches contain the original workspace path. Generate a fresh build directory:

```powershell
$cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake -S . -B build-restored -G 'Visual Studio 17 2022' -A x64
& $cmake --build build-restored --config Debug
& $cmake --build build-restored --config Release
python tests\tms6747-simulator\qualify-own.py --compiler-build build-restored --tools-root toolsets\shalimar-c6747 --rts-lib toolsets\shalimar-c6747\lib\rts6x.lib
python tests\tms6747-simulator\qualify.py --compiler-build build-restored
```

The reference handoff is retained as `docs/TAKE-UP-NOW_REFERENCE.txt`; current user requests, including independent tool selection, control the work. Original source installations and earlier archives were left intact.
