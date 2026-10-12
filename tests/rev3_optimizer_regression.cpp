#include "Lexer.h"
#include "Parser.h"
#include "SemanticChecker.h"
#include "IR.h"
#include <cassert>
#include <string>
#include <iostream>
int main() {
    using namespace shalimar;
    const std::string source = "fun <int> = compare(x: int, y: int) {\n"
        "if x < y { if x <= y { return 42 } }\nreturn 99\n}\nfun main() {}\n";
    std::vector<Token> tokens; std::string error; Program ast; IRProgram ir;
    ir.sourceFile = "optimizer-regression.shl";
    assert(Lexer().tokenize(source, tokens, error));
    assert(Parser(ast).parse(tokens, ast, error));
    assert(SemanticChecker().check(ast, error));
    assert(IRBuilder().build(ast, ir, error));
    IROptimizer().optimize(ir);
    if (!IRVerifier().verify(ir, error)) { std::cerr << error << '\n'; return 1; }
    const IRFunction& f = ir.functions[0];
    assert(f.blocks[0].instructions[0].opcode == "binary <");
    assert(f.blocks[0].instructions[1].opcode == "branch");
    const int thenBlock = f.blocks[0].successors[0];
    assert(f.blocks[thenBlock].instructions[0].opcode == "binary <=");
    assert(f.blocks[thenBlock].instructions[1].opcode == "branch");
    return 0;
}
