# rev49 P0: IR expression narrowing checkpoint

The IR builder now uses direct checked C++98 RTTI narrowing (`dynamic_cast` to const pointers) for eight expression-node categories when lowering expressions: literal, name, unary, binary, index, attribute, array literal, and call. This removes eight uses of the custom `astCast<T>` helper from the main expression-dispatch path. The literal formatting helper now accepts `const LiteralExpr&` rather than an `AstRef<const LiteralExpr>`.

This is an intermediate checkpoint, **not** completion of the named-ID migration: `AstRef<T>` and `astCast<T>` still occur elsewhere, including the IR builder. Follow-up work must replace AST relationships and caller interfaces with named node IDs and checked arena access.

Verification: strict C++98 build and the rev49 structured-return-layout, native real addition, and native real subtraction regression scripts passed. Native Windows object generation was inspected by the existing tests; no claim of Windows runtime execution or four-target readiness is made.
