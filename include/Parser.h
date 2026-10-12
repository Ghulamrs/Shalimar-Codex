#ifndef SHALIMAR_PARSER_H
#define SHALIMAR_PARSER_H

#include "Ast.h"
#include "Token.h"

#include <string>
#include <vector>

namespace shalimar {

class Parser {
public:
    explicit Parser(Program& program) : arena(program.arena), pos(0) {}
    bool parse(const std::vector<Token>& input, Program& program, std::string& error);

private:
    AstArena& arena;
    std::vector<Token> tokens;
    size_t pos;
    std::string error;

    const Token& peek(size_t lookahead = 0) const;
    bool match(TokenKind kind);
    bool expect(TokenKind kind, const std::string& description);
    void skipLines();
    bool atStatementEnd() const;
    int precedence(TokenKind kind) const;
    bool looksLikeMultiAssign() const;
    AstRef<Expr> parseExpression(int minPrecedence = 0);
    AstRef<Expr> parsePrimary();
    AstRef<Expr> parseInitializer();
    AstRef<BlockStmt> parseBlock();
    AstRef<Stmt> parseStatement();
    AstRef<FunctionDef> parseFunction();
};

} // namespace shalimar

#endif // SHALIMAR_PARSER_H
