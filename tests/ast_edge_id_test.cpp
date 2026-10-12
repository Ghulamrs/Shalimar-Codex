#include "Ast.h"
#include <cassert>
#include <stdexcept>
using namespace shalimar;
int main() {
    Program p;
    LiteralExprId literal = p.arena.addId(LiteralExpr("123"));
    NameExprId name = p.arena.addId(NameExpr("x"));
    ExprId a = p.arena.edge(literal);
    ExprId b = p.arena.edge(name);
    assert(dynamic_cast<LiteralExpr&>(p.arena.node(a)).value == "123");
    assert(dynamic_cast<NameExpr&>(p.arena.node(b)).name == "x");
    assert(p.arena.node(p.arena.asLiteralExpr(a)).value == "123");
    assert(p.arena.node(p.arena.asNameExpr(b)).name == "x");
    bool mismatch = false;
    try { p.arena.asNameExpr(a); } catch (const std::logic_error&) { mismatch = true; }
    assert(mismatch);

    for (int i = 0; i < 10000; ++i) p.arena.addId(LiteralExpr("padding"));
    assert(dynamic_cast<LiteralExpr&>(p.arena.node(a)).value == "123");
    BlockStmtId block = p.arena.addId(BlockStmt());
    StmtId statement = p.arena.edge(block);
    assert(&p.arena.node(p.arena.asBlockStmt(statement)) == &p.arena.node(block));
    assert(dynamic_cast<BlockStmt*>(&p.arena.node(statement)) != 0);
    bool rejected = false;
    try { p.arena.node(ExprId()); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { p.arena.node(StmtId()); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    Program other;
    mismatch = false;
    try { other.arena.asLiteralExpr(a); } catch (const std::logic_error&) { mismatch = true; }
    assert(mismatch);
    rejected = false;
    try { other.arena.node(a); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { other.arena.edge(literal); } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { p.arena.node(ExprId(statement.pool_, statement.index_, statement.arenaId_)); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    return 0;
}
