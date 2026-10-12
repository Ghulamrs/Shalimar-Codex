# Language coverage repair queue

The user authorized implementation and completion item by item, without repeated confirmation. Existing language and array decisions remain the contract. Each implementation item requires execution/error tests and debugger evidence where supported; four-target completion requires all four native pipelines.

| Order | Item | Current state |
| --- | --- | --- |
| 1 | Local one-dimensional integer arrays | Implemented restricted Windows/DSP subset; 192 runtime cases pass |
| 2 | Local one-dimensional real arrays | Implemented within the same storage/selection limits; 248 runtime cases and Windows debugger inspection pass |
| 3 | Local char buffers | Next: preserve capacity/text-length distinction, one string terminator, no truncation, zero default initialization and writable last slot |
| 4 | Multidimensional arrays and row operations | Pending: exact shapes, indexing/bounds, copies, layout and debugger dimensions |
| 5 | Globals and array parameters/references | Pending: symbol storage, initialization, ownership/calling convention and debugger locations |
| 6 | General control flow and calls with arrays | Pending: remove Windows single-block/call restrictions through typed selection and scope-aware variable identities |
| 7 | Remaining scalar/operators/runtime constructs | Pending: inventory against normative specification, implement checked semantics and required diagnostics |
| 8 | Complete debugger coverage | Partial Windows/TI support; scopes/types/all prologues, DSP array descriptions and SIM6747 interactive integration remain |
| 9 | Linux x64 native pipeline | Pending: ELF, System V ABI, runtime, execution and DWARF/debugger qualification |
| 10 | macOS ARM64 native pipeline | Pending: Mach-O, Darwin ABI, runtime, execution and DWARF/debugger qualification |
| 11 | Optimization and full conformance gate | Pending: effects/traps/typed IR, measured target-specific optimizations and original examples end to end |

Items 1 and 2 are implemented subsets, not complete array-language conformance. See NATIVE_INT_ARRAY_QUALIFICATION.md, NATIVE_REAL_ARRAY_QUALIFICATION.md and WINDOWS_DEBUGGER_QUALIFICATION.md for exact limits and reproducible tests. The compiler is not yet a complete release, and frontend acceptance never substitutes for native execution or debugger qualification.
