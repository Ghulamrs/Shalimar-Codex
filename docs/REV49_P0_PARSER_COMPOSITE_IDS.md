# rev49 P0 parser composite-expression IDs

Unary, index and array-attribute expressions are now created using concrete `UnaryExprId`, `IndexExprId`, and `AttributeExprId` values, with checked arena access before temporarily adapting to legacy AST edges. This is a transitional change; `AstRef<T>` remains widely used. It does not claim full AST migration or native-target readiness.
