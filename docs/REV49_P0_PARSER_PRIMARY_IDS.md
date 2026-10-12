# rev49 P0: named primary expression IDs

Parser primary literal and name expressions now originate from `LiteralExprId` and `NameExprId` via `addId`, with checked arena access before conversion to temporary legacy references. Both integer/real and string literals are covered. The rest of the parser, semantic checker and IR builder still depend on `AstRef<T>`; this is not completion of the migration.
