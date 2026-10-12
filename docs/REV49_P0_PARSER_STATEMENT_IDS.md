# rev49 P0 parser statement IDs

Print and return statement construction now starts with non-template PrintStmtId and ReturnStmtId, and accesses statement fields through checked arena.node(id). The legacy AstRef compatibility bridge remains at the parseStatement interface. Full removal is pending.
