#include "Ast.h"
#include <iostream>
#include <stdexcept>
using namespace shalimar;
static int failures = 0;
static void expectFailure(AstArena& arena, const ExprId& id) {
    try { arena.node(id); ++failures; }
    catch (const std::exception&) {}
}
int main() {
    AstArena first;
    AstArena second;
    LiteralExprId literal = first.addId(LiteralExpr("12"));
    ExprId expression = first.edge(literal);
    if (first.node(literal).value != "12") ++failures;
    expectFailure(second, expression);
    expectFailure(first, ExprId());
    expectFailure(first, ExprId(0, 999999, first.arenaIdentity()));
    expectFailure(first, ExprId(8, 0, first.arenaIdentity()));
    try { first.asNameExpr(expression); ++failures; }
    catch (const std::exception&) {}
    try { first.node(first.asLiteralExpr(expression)); }
    catch (const std::exception&) { ++failures; }
    if (failures) { std::cerr << failures << " AST-ID safety failures\n"; return 1; }
    std::cout << "AST-ID safety checks passed\n";
    return 0;
}
