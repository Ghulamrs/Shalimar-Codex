# Shalimar rev6: verified results and limits

Date: 2026-10-11. This report supersedes earlier implementation-status claims in the supplied archive. It distinguishes successful compiler refactoring, tested language behavior, and remaining conformance work.

## Result

The container-owned indexed AST refactoring is implemented throughout the compiler. All 12 supplied programs, unchanged, pass checking, native compilation, execution, and their supplied output expectations through the default AST-to-C backend on Linux x86-64. Both compiler configurations and both Shalimar modes pass: 48/48 cases.

This is not a claim that every rule in the 66 KB normative reference is implemented. The experimental IR-native backend has narrower coverage, and generated-program memory management remains separate from compiler memory management.

## Revision checkpoints

See REVISION_HISTORY.md. Every revision ZIP contains a complete project, source, original programs, Linux compiler, and checkpoint evidence. rev6 is the current revision. The older checkpoint compilers intentionally retain their earlier defects.

## Debug/Release matrix

| Compiler build | Shalimar mode | Default backend output checks |
| --- | --- | --- |
| Debug (`-g -O0`, C++98) | Debug | 12/12 |
| Debug (`-g -O0`, C++98) | Release | 12/12 |
| Release (`-O2 -DNDEBUG`, C++98) | Debug | 12/12 |
| Release (`-O2 -DNDEBUG`, C++98) | Release | 12/12 |

The same original program produces byte-identical output in all four combinations. The runner compares every supplied expected line. It ignores column-padding and trailing-space differences and permits only the rounding tolerance implied by expected decimal digits. table.shm supplies selected rows; a separate multiplication oracle verifies all ten rows. Exact added conformance tests verify actual trailing spaces, six-place default grids, precision overrides, adjacent statements, else-if chains, inferred strings, constant shadowing, real loops, and short-form loop sugar.

Both compiler builds pass the existing full smoke suite. Original .shm files were not rewritten. Their SHA-256 values agree with the uploaded test archive. One invalid synthetic smoke fixture was moved to a valid print line so it tests its intended semantic error; smoke comparisons trim normative trailing spaces. The exact conformance tests do not trim them.

Shalimar Release runs the AST optimizer before output generation; the IR path also runs its IR optimization passes. Existing checked-arithmetic, optimizer, expression-precedence, string, array, and reference regressions remain passing. Debug-generated Linux objects contain `.debug_info`; Release-generated objects do not. `--object` was exercised in both modes, not only `--emit-c`.

## Compiler memory-management audit

- Zero explicit `new`, `delete`, `malloc`, `calloc`, `realloc`, or `free` operations in executable compiler source tokens under src/ and include/.
- AST nodes are stored by value in typed std::deque pools. Child relationships and roots are AstRef handles containing a pool identifier and integer index.
- Parser tokens are stored by value in std::vector<Token>. Symbols, IR instructions, and blocks use standard containers.
- Pool lookup checks indices and dynamic types. Program owns the pools and is noncopyable. Handle copies do not acquire ownership and nodes are never erased or reordered.
- Observer pointers remain inside the handle resolver, CLI/C-library interfaces, and some borrowed context fields. These pointers do not own AST nodes. This is not a promise of zero pointer syntax anywhere.
- Standard containers allocate internally. The achieved rule is container-managed compiler memory, not literally static memory with no heap allocation.
- The 100,000-node handle-growth/lifetime test and all 12 original compiler runs pass AddressSanitizer/UndefinedBehaviorSanitizer with leak detection disabled. LeakSanitizer itself cannot run in this environment because it cannot inspect the process tasks; no leak-scan result is claimed.

The audit separately reports 18 allocation-related string literals that generate C runtime operations. Those are existing/generated-program runtime allocation helpers, not compiler allocation substitutions. Generated Shalimar executables still use C allocation for runtime-sized arrays and string operations. A requirement to remove those operations too would require a separate generated-runtime/backend representation change. The report does not hide them or claim their removal.

## Backend coverage of the supplied programs

| Original program | AST-to-C (both modes) | Experimental IR-native (Debug and Release) |
| --- | --- | --- |
| factorial.shm | Pass | Pass |
| fibonacci.shm | Pass | Pass |
| gaussseidel.shm | Pass | Rejected: unsupported IR features |
| gcd.shm | Pass | Pass |
| invert.shm | Pass | Rejected: unsupported IR features |
| prime.shm | Pass | Rejected: unsupported IR features |
| quadratic.shm | Pass | Rejected: unsupported IR features |
| rotations.shm | Pass | Rejected: unsupported IR features |
| rotmat.shm | Pass | Rejected: unsupported IR features |
| sqroot.shm | Pass | Rejected: unsupported IR features |
| strsplit.shm | Pass | Rejected: unsupported IR features |
| table.shm | Pass | Pass |

IR-native passes factorial, fibonacci, gcd, and table. The remaining eight stop with an unsupported-feature diagnostic. Borrowed math functions other than len, real arrays, multiple outputs, real loop counters, inferred-capacity strings, and some control-flow/array forms are not fully implemented there. Its default loop start/step and print spacing were repaired, but parity is not claimed. No automatic fallback disguises an IR failure as native-IR success.

## Platform verification

| Platform | Project/configuration status | Native verification |
| --- | --- | --- |
| Linux x86-64 | C++98 Debug/Release compiler builds and both Shalimar modes | Passed; binaries included |
| Windows | Visual Studio 2022 solution, Debug/Release settings, explicit Debug compiler symbols | Not run on Windows; no new Windows executable included |
| macOS | Xcode workspace/project, explicit Debug symbols and optimization level, Release optimization | Not run on macOS; no new macOS executable included |

MSVC's project selects its supported C++14 mode because it has no C++98 switch; source-language compatibility is verified separately with g++ -std=c++98. Xcode may build a native macOS backend compiler from this source; a Windows .exe cannot serve as its native compiler executable.

## Scope still requiring work before whole-language completion

The supplied programs are verified, not an exhaustive language proof. Broader normative-reference reconciliation and coverage remain necessary, including all array-copy forms, declaration lifetime/redeclaration rules, character conversion/arithmetic rules, numeric range handling at every implicit conversion, and cross-platform runtime behavior. Historical Swift implementation notes in SHALIMAR_LANGUAGE.md do not describe this C++ compiler's representation. This revision does not silently weaken the reference to make tests pass.

## Repeat the checks

On Linux with g++, cc, Python 3, and a POSIX shell:

```sh
sh tests/run-debug-release.sh
python3 tests/audit-allocation.py
```

The matrix script builds the actual compiler twice, tests all four mode combinations, runs the exact new conformance checks and both full smoke regressions, and exercises the indexed arena. To exercise the experimental backend separately, run tests/run-original.py with --backend ir; its nonzero status is expected while the limitations above remain.

See docs/validation/rev6 for machine-readable results and logs, and tests/original/SHA256.json for unchanged-source evidence.
