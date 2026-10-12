#include "Ast.h"
#include <cassert>
#include <sstream>
#include <stdexcept>

int main() {
    using namespace shalimar;
    Program first;
    Program second;
    AstRef<LiteralExpr> literal = first.arena.add(LiteralExpr("42"));
    AstRef<Expr> root = literal;
    for (int i = 0; i < 100000; ++i) {
        root = first.arena.add(UnaryExpr("-", root));
        first.arena.add(NameExpr("growth"));
    }
    assert(literal->value == "42");
    assert(!astCast<LiteralExpr>(root));
    AstRef<LiteralExpr> other = second.arena.add(LiteralExpr("99"));
    assert(other->value == "99" && literal->value == "42");
    AstRef<const Expr> readOnly = literal;
    assert(astCast<const LiteralExpr>(readOnly)->value == "42");
    bool caught = false;
    try {
        AstRef<LiteralExpr> invalid(first.arena, literal.pool(), 99999999);
        (void)invalid->value;
    } catch (const std::out_of_range&) { caught = true; }
    assert(caught);
    caught = false;
    try { AstRef<Expr> empty; (void)*empty; }
    catch (const std::logic_error&) { caught = true; }
    assert(caught);
    return 0;
}
