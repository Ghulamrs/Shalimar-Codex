# rev49 P0 parser declaration and multi-assignment IDs

Parser statement construction now uses named `DeclStmtId` and `MultiAssignStmtId` identifiers for local declarations and multi-result assignments. Fields are accessed through `arena.node(id)`; the existing `AstRef` return bridges remain until parser interfaces and consumers are migrated. This does not complete removal of `AstRef<T>`.

Verification: strict C++98 compiler build, typed return-layout test, and Debug/Release native real-parameter subtraction test.
