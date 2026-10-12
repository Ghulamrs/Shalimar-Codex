# Shalimar Language Specification Freeze Policy

**Specification baseline:** Shalimar 3.0 language rules in `SHALIMAR_LANGUAGE.md`  
**Status:** language design baseline frozen; compiler conformance remains incomplete  
**Date:** 2026-10-11

## Authority

`SHALIMAR_LANGUAGE.md` is the canonical source for the intended language-level syntax and semantics. Its language rules, grammar, precedence table, type rules, declaration rules, function/array/string rules, built-ins, diagnostics, and limits define the target language.

Sections explicitly labeled **Implementation note**, **Architecture map**, or describing the Swift interpreter/editor explain that reference implementation. They do not require this C++ compiler to use Swift or reproduce the interpreter's internal architecture. They may, however, describe observable language behavior that the compiler should match.

The compiler's current behavior and limitations are recorded separately in `STATUS.md` and `FEATURE_PARITY.md`. A mismatch is an implementation gap unless a deliberate language-design change is approved.

## Freeze rules

1. Do not silently change the grammar or observable semantics while implementing a feature.
2. If implementation evidence reveals an ambiguity, record it in `SPEC_QUESTIONS.md` and resolve it before changing the canonical rule.
3. Any approved language-level change must update the canonical reference, a small executable example, and regression tests in the same change.
4. A feature is not considered implemented merely because the parser accepts it. It must pass semantic checking, generate correct code, and have a runtime regression test.
5. Backend-specific unsupported constructs must be rejected with a clear diagnostic; the IR-native backend must not silently fall back to AST-to-C.
6. The language can be called frozen independently of compiler completion. A release may claim conformance only to the explicitly tested feature set.

## Versioning

The current target is Shalimar 3.0 as described in `SHALIMAR_LANGUAGE.md`. Until the open questions in `SPEC_QUESTIONS.md` are resolved, the compiler should not claim full 3.0 conformance. A breaking syntax or semantic change requires a new language-version decision and a corresponding reference update.
