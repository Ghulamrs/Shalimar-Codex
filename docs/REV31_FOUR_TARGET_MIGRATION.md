# rev31: Four-target legacy migration and restricted S-mode selector

## Implemented
- Preserved the uploaded legacy backend sources under `legacy/backend_reference/` for audited, gradual porting. They are NOT compiled into the current compiler; legacy source uses post-C++98 features.
- Added a C++98 `x86_64::IRSelector` that accepts only a single basic block with integer constant additions/subtractions and one integer return, with no inputs. It emits machine-code bytes through the rev30 encoder, without generating C. Unsupported IR is rejected explicitly.
- Added a C++98 TI TMS6747 target contract and a test for target identity. Object format and debugger compatibility are deliberately marked unverified.
- Integrated the new files in CMake and added an isolated selector regression test.

## Not yet implemented
- S-mode CLI object emission, COFF writer, relocations, Windows ABI, native debugger symbols, general IR selection, DSP instruction emission, TI object/debug formats, and cross-platform toolchain validation.
- This restricted selector is a backend library building block, not a claim that S-mode programs execute natively.

## Priorities
Windows x86-64 -> Linux x86-64 -> macOS ARM64 -> TI TMS6747, with DSP architectural work maintained alongside the others.
