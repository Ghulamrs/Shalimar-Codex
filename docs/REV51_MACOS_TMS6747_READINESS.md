# rev51 target readiness: macOS ARM64 and TI TMS6747

The rev51 multiple-return work currently generates restricted Windows x86-64 COFF code. Its caller-owned result layout is conceptually reusable across targets, but the hidden result pointer's ABI placement, stack alignment, and aggregate alignment must be lowered separately for each target.

## macOS ARM64

The source tree includes `src/backend/legacy_port/Arm64Darwin.cpp`, but this is legacy-port material, not proof of an integrated S-native Mach-O object writer. To qualify, implement ARM64 instruction selection, the Darwin ABI for hidden multi-result pointers, Mach-O relocations, DWARF locations, and compile/link/run tests on an ARM64 macOS machine. Do not claim success based on compiling the legacy file.

## TI TMS6747

The source tree includes `src/backend/legacy_port/Tms6747.cpp` and `src/backend/tms6747/TargetContract.cpp`. The contract explicitly labels the object/debug formats unverified. Qualification requires the actual TMS6747 ABI (including result pointer passing), DSP instruction emission, relocatable objects accepted by the TI toolchain, and execution in a supported simulator or board, with Debug and Release checks.

## rev51 boundary

No macOS or TI native execution has been demonstrated. These targets remain rev56 and rev57 deliverables respectively; rev51 may establish portable typed multi-return IR, but must not imply target backend completion. A presence/claims audit is available at `tests/run-rev51-target-readiness.sh`.
