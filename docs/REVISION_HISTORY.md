# Revision history and checkpoint policy

Current checkpoint: TMS6747 Windows Merge Rev3, 11 October 2026. Added independent ASM6x/LNK6x/RTS6x selection and 48/48 own-tool VM/SIM/CCS cases, a separately verified TI/DWARF debugger path, Release CFG/dominance repairs, and the Windows real-subtraction fix. Read [TMS6747_REV3_QUALIFICATION.md](TMS6747_REV3_QUALIFICATION.md) for exact coverage and remaining gaps. Earlier entries below are historical.

These revisions are cumulative: each checkpoint ZIP is a complete project, not a patch. Use rev6 for current work. All older checkpoints intentionally preserve their then-current limitations.

The attached project predates the rev1 described in the supplied conversation. This run recreated the parser-token ownership checkpoint from the actual supplied bytes. The new rev1 is not claimed to be byte-identical to an unavailable earlier rev1 archive.

The previous roadmap boundaries were revised: the indexed AST changed public types, so its parser, checker, optimizer, IR, and generator adaptations were committed together as the buildable rev2 checkpoint. Language repairs were then separated into parser, semantic, output, and validation revisions.

| Revision | Concrete work | Existing smoke regression | Original expected-output tests, Debug mode |
| --- | --- | --- | --- |
| rev1 | Parser owns its token vector; repeatable smoke runner accepts an existing compiler. | Passed | 0/12 |
| rev2 | Indexed, typed container AST pools; parser, semantic checker, optimizer, IR builder, and both code generators use non-owning index handles. Explicit compiler allocations removed. | Passed | 0/12 |
| rev3 | Layout-independent parsing, multiline parameters/literals, adjacent statements, whitespace print lists, line placement rule, and multi-assignment boundary recognition. | Passed | 1/12 |
| rev4 | Canonical typed declarations for first assignments and literal arrays; fresh multi-output variables; pi/e constants; original programs all pass checking. | Passed | 6/12 |
| rev5 | Numeric grid printing, scalar/grid precision defaults, trailing print spaces and ?? behavior, inferred-capacity strings, and AST/IR print spacing parity. | Passed | 12/12 |
| rev6 | Real-valued AST loop counters, short-form sugar, zero-step checks, real remainder generation, chained else-if, constant annotations shared with IR, IR default loop steps, explicit unsupported-IR diagnostics, Debug/Release verification, and source audit. | Passed in Debug and Release | 12/12 in all four combinations |

For rev1–rev5, both Shalimar Debug and Release modes were also tested with that checkpoint's Linux Debug compiler. Their original-test pass counts agree. Smoke tests pass at each checkpoint.

No original .shm source was altered. Its SHA-256 digests are in tests/original/SHA256.json. The old smoke harness trims newly required trailing spaces for existing comparisons, and its one synthetic inline-print fixture was moved onto a valid print line so it still reaches the intended dimension error. Exact new conformance checks independently assert spaces, grid precision, and the print-placement rule.
