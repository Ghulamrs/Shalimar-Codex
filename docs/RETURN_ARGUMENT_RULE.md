# Return argument expression rule (rev51–rev52)

In a parenthesized return list, each comma at the outermost nesting level separates one result. Each result may be a literal, identifier, or ordinary non-call expression. The normal expression evaluator handles operators. **Function calls are forbidden at any depth inside any parenthesized return argument**, including conversion-call syntax such as `int(x)`.

Valid: `return (x + 1, (y * 2) - 3)`.
Invalid: `return (calculate(x), y)` and `return (x + int(y), 2)`.

The semantic checker recursively walks each return argument and rejects nested `CallExpr` nodes before normal type validation. The parser records whether the return was parenthesized; bare `return expression` is unaffected by this rule. Tests: `tests/return_arithmetic_no_calls_valid.shl`, `tests/return_no_nested_calls_invalid.shl`, `tests/return_direct_call_invalid.shl`.

This is a frontend rule, not proof that all permitted expressions are already lowered by each native target backend.
