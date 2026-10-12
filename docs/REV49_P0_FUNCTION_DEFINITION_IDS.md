# rev49 P0: Function-definition construction via named IDs

`Parser::parseFunction()` now creates `FunctionDefId` through `AstArena::addId` and accesses its fields through checked `arena.node(functionId)`. This covers source line, function name, return types, parameters, and body. The existing `AstRef<FunctionDef>` return boundary remains as a temporary compatibility bridge until parser callers, semantic checking, IR building, and AST relationships have been migrated.

Verification: strict C++98 build; rev49 return-layout, real-parameter addition and subtraction Debug/Release tests; conformance matrix (24 PASS, 28 GAP, 1 RECORDED). This checkpoint does not complete P0 or any native target.
