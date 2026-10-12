# Windows debugger implementation checkpoint

Rev3 now emits CodeView array types and verified frame-relative locations for the supported local numeric-vector selection path. Array types describe actual four-byte integer or eight-byte real elements and storage extents. Parameters and initialized scalar locals in that path also receive locations. Live ranges start after the selector establishes initialized storage and end before frame teardown.

Live locations name RBP explicitly through S_DEFRANGE_REGISTER_REL. Previously emitted implicit frame-pointer ranges depended on frame metadata that was absent. The COFF writer also corrects line-block sizes to include their headers, and collapses adjacent source marks for the same source line into one source stop.

Windows runtime-function and unwind sections are emitted for the existing exact `push rbp; mov rbp,rsp; sub rsp,imm32` prologue with a nonzero, aligned frame. Both small and large allocation unwind encodings are supported. The writer preserves these sections independently of the debug-symbol option. Other prologue shapes are not covered by this checkpoint.

## Executed qualification

Run `python tests/run-windows-array-debugger.py --compiler-build PATH --output DIRECTORY`. The build must contain Debug/Release shalimarc.exe and shalimar_runtime.lib. The runner uses the locally installed Microsoft linker, Windows SDK, CDB and Python. Their concrete installation paths are recorded in the script.

Eight CDB scenarios pass: two host configurations, two source modes, and first/last element inputs. Each scenario links a PDB, resolves a breakpoint from an absolute Shalimar source path and line, inspects four distinct array types/extents and ten elements, checks the parameter and scalar local, obtains a caller stack, steps out, and continues to the expected result. The observed arrays are int[3] {10,20,30}, int[2] {7,9}, real[3] {1.25,2.5,3.75} and real[2] {0.1,0.2}. CDB displays Shalimar real through CodeView's double type.

Sixty independent RtlVirtualUnwind checks pass across those host/source combinations and mixed-type, small and large frame fixtures. These cover function entry, partially executed prologues, the body and additional temporary stack allocation. They verify restored caller RSP, RBP and return PC. The repeated per-input entries in results.json reference the same checks; they are not additional independent unwind tests.

After the implementation, native integer-array runtime regressions pass 192/192, existing Windows runtime checks pass 108/108, and DSP checks pass 52/52. C++ contracts pass 14/14 in each host configuration. Workspace evidence is compiler-review/debugger/results.json and its CDB logs, compiler-review/native-int-arrays/results-all.json, and compiler-review/repair-native.

## Remaining release requirements

This qualifies a restricted Windows subset. It does not establish all-language or four-target debugger parity. Lexical shadowing, source columns, complete function/parameter type descriptions, other array forms, all other prologues and optimized location changes still need implementation and dedicated debugger tests. The array backend's existing single-block and scalar input/output limits remain.

SIM6747 interactive debugging and the bundled DSP toolchain's debug-section support remain separate unfinished implementation work. Linux/macOS native pipelines and on-target debugger qualification are also unfinished. See DEBUGGER_RELEASE_REQUIREMENTS.md.
