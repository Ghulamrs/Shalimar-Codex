#include "Ast.h"
#include <cassert>
#include <stdexcept>
using namespace shalimar;
int main() {
    Program p;
    AstRef<LiteralExpr> old = p.arena.add(LiteralExpr("42"));
    LiteralExprId id = p.arena.id(old);
    assert(id.valid());
    assert(p.arena.node(id).value == "42");
    for (int i = 0; i < 10000; ++i) p.arena.add(LiteralExpr("other"));
    assert(p.arena.node(id).value == "42");
    const AstArena& readonly = p.arena;
    assert(readonly.node(id).value == "42");
    bool rejected = false;
    try { p.arena.node(LiteralExprId()); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    Program other;
    rejected = false;
    try { other.arena.node(id); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { other.arena.id(old); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    NameExprId direct = p.arena.addId(NameExpr("direct"));
    assert(p.arena.node(direct).name == "direct");
    // Named IDs preserve exact node kind; no template-based cast is needed.
    rejected = false;
    try { other.arena.node(direct); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    return 0;
}
