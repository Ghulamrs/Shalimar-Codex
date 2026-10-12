#include "Parser.h"

#include <sstream>

namespace shalimar {

namespace {

std::string numberToString(int value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

bool isTypeToken(const Token& token) {
    return token.kind == TK_KEYWORD &&
           (token.text == "int" || token.text == "real" || token.text == "char");
}

} // namespace

const Token& Parser::peek(size_t lookahead) const {
    size_t index = pos + lookahead;
    if (index >= tokens.size()) {
        return tokens.back();
    }
    return tokens[index];
}

bool Parser::match(TokenKind kind) {
    if (peek().kind != kind) {
        return false;
    }
    ++pos;
    return true;
}

bool Parser::expect(TokenKind kind, const std::string& description) {
    if (match(kind)) {
        return true;
    }
    error = "line " + numberToString(peek().line) + ": expected " +
            description + ", found '" + peek().text + "'";
    return false;
}

void Parser::skipLines() {
    while (match(TK_NEWLINE)) {
    }
}

bool Parser::atStatementEnd() const {
    return peek().kind == TK_RBRACE || peek().kind == TK_EOF ||
           (pos > 0 && peek().line > tokens[pos - 1].line);
}

int Parser::precedence(TokenKind kind) const {
    switch (kind) {
    case TK_OR: return 1;
    case TK_AND: return 2;
    case TK_EQ:
    case TK_NE:
    case TK_LT:
    case TK_GT:
    case TK_LE:
    case TK_GE: return 3;
    case TK_PLUS:
    case TK_MINUS: return 4;
    case TK_STAR:
    case TK_SLASH:
    case TK_PERCENT: return 5;
    case TK_CARET: return 6;
    default: return -1;
    }
}

bool Parser::looksLikeMultiAssign() const {
    if (peek().kind != TK_LT || peek(1).kind != TK_IDENT) {
        return false;
    }
    size_t index = pos + 2;
    while (index < tokens.size() && tokens[index].kind == TK_COMMA) {
        if (index + 1 >= tokens.size() ||
            tokens[index + 1].kind != TK_IDENT) {
            return false;
        }
        index += 2;
    }
    if (index + 2 >= tokens.size() ||
        tokens[index].kind != TK_GT ||
        tokens[index + 1].kind != TK_COLON ||
        tokens[index + 2].kind != TK_IDENT) {
        return false;
    }
    return index + 3 < tokens.size() &&
           tokens[index + 3].kind == TK_LPAREN;
}

AstRef<Expr> Parser::parsePrimary() {
    const Token token = peek();
    AstRef<Expr> expression = 0;

    if (match(TK_INT) || match(TK_REAL)) {
        expression = arena.legacyLiteralRef(arena.addId(LiteralExpr(token.text)));
    } else if (match(TK_STRING)) {
        expression = arena.legacyLiteralRef(arena.addId(LiteralExpr(token.text, true)));
    } else if (match(TK_MINUS)) {
        AstRef<Expr> operand = parsePrimary();
        if (!operand) {
            return 0;
        }
        expression = arena.legacyUnaryRef(arena.addId(UnaryExpr("-", operand)));
    } else if (match(TK_LPAREN)) {
        expression = parseExpression();
        if (!expression || !expect(TK_RPAREN, "')'")) {

            return 0;
        }
    } else if (peek().kind == TK_IDENT || isTypeToken(peek())) {
        ++pos;
        const std::string name = token.text;
        if (match(TK_LPAREN)) {
            CallExprId call = arena.addId(CallExpr(name));
            if (peek().kind != TK_RPAREN) {
                do {
                    AstRef<Expr> argument = parseExpression();
                    if (!argument) {

                        return 0;
                    }
                    arena.node(call).args.push_back(argument);
                } while (match(TK_COMMA));
            }
            if (!expect(TK_RPAREN, "')'")) {

                return 0;
            }
            expression = arena.legacyCallRef(call);
        } else {
            expression = arena.legacyNameRef(arena.addId(NameExpr(name)));
        }
    } else {
        error = "line " + numberToString(token.line) + ": expected expression";
        return 0;
    }

    while (true) {
        if (match(TK_LBRACKET)) {
            AstRef<Expr> index = parseExpression();
            if (!index || !expect(TK_RBRACKET, "']'")) {

                return 0;
            }
            expression = arena.legacyIndexRef(arena.addId(IndexExpr(expression, index)));
            continue;
        }

        if (match(TK_DOT)) {
            if (peek().kind != TK_IDENT) {

                error = "line " + numberToString(peek().line) +
                        ": expected array attribute after '.'";
                return 0;
            }
            const std::string attribute = peek().text;
            ++pos;
            AstRef<Expr> axis = 0;
            if (attribute == "dim") {
                if (!expect(TK_LPAREN, "'(' after .dim")) {

                    return 0;
                }
                axis = parseExpression();
                if (!axis || !expect(TK_RPAREN, "')' after .dim axis")) {

                    return 0;
                }
            } else if (attribute != "row" && attribute != "col") {

                error = "line " + numberToString(peek().line) +
                        ": unknown array attribute '" + attribute + "'";
                return 0;
            }
            expression = arena.legacyAttributeRef(arena.addId(AttributeExpr(expression, attribute, axis)));
            continue;
        }

        break;
    }
    return expression;
}

AstRef<Expr> Parser::parseExpression(int minPrecedence) {
    AstRef<Expr> left = parsePrimary();
    if (!left) {
        return 0;
    }

    while (true) {
        if (looksLikeMultiAssign()) break;
        const int operatorPrecedence = precedence(peek().kind);
        if (operatorPrecedence < minPrecedence) {
            break;
        }
        const Token operation = peek();
        ++pos;
        const int nextPrecedence = operatorPrecedence +
            (operation.kind == TK_CARET ? 0 : 1);
        AstRef<Expr> right = parseExpression(nextPrecedence);
        if (!right) {

            return 0;
        }
        BinaryExprId binaryId = arena.addId(BinaryExpr(operation.text, left, right));
        left = AstRef<BinaryExpr>(arena, binaryId.pool(), binaryId.index());
    }
    return left;
}

AstRef<Expr> Parser::parseInitializer() {
    if (!match(TK_LBRACE)) {
        return parseExpression();
    }

    ArrayLiteralExprId arrayId = arena.addId(ArrayLiteralExpr());
    while (peek().kind != TK_RBRACE && peek().kind != TK_EOF) {
        if (peek().kind == TK_COMMA) {
            // An empty slot is a zero-initialized element in Shalimar.
            arena.node(arrayId).values.push_back(0);
        } else {
            AstRef<Expr> value = parseInitializer();
            if (!value) {

                return 0;
            }
            arena.node(arrayId).values.push_back(value);
        }

        if (!match(TK_COMMA)) {
            break;
        }
        if (peek().kind == TK_RBRACE) {
            // A final empty slot is permitted by the language's Slot grammar.
            arena.node(arrayId).values.push_back(0);
            break;
        }
    }
    if (!expect(TK_RBRACE, "'}' ending array literal")) {

        return 0;
    }
    return AstRef<ArrayLiteralExpr>(arena, arrayId.pool(), arrayId.index());
}

AstRef<BlockStmt> Parser::parseBlock() {
    if (!expect(TK_LBRACE, "'{'")) {
        return 0;
    }
    BlockStmtId blockId = arena.addId(BlockStmt());
    skipLines();

    while (peek().kind != TK_RBRACE && peek().kind != TK_EOF) {
        AstRef<Stmt> statement = parseStatement();
        if (!statement) {

            return 0;
        }
        arena.node(blockId).statements.push_back(statement);
        skipLines();
    }

    if (!expect(TK_RBRACE, "'}'")) {

        return 0;
    }
    return AstRef<BlockStmt>(arena, blockId.pool(), blockId.index());
}

AstRef<Stmt> Parser::parseStatement() {
    skipLines();
    const Token token = peek();

    if (isTypeToken(token)) {
        ++pos;
        DeclStmtId declaration;
        if (!expect(TK_IDENT, "variable name")) {
            return 0;
        }
        declaration = arena.addId(DeclStmt(token.text, tokens[pos - 1].text, token.line));
        while (match(TK_LBRACKET)) {
            AstRef<Expr> dimension = parseExpression();
            if (!dimension || !expect(TK_RBRACKET, "']' after array dimension")) {

                return 0;
            }
            arena.node(declaration).dimensions.push_back(dimension);
        }
        if (match(TK_COLON) || match(TK_EQ)) {
            arena.node(declaration).init = parseInitializer();
            if (!arena.node(declaration).init) {

                return 0;
            }
        }
        return AstRef<DeclStmt>(arena, declaration.pool(), declaration.index());
    }

    if (looksLikeMultiAssign()) {
        MultiAssignStmtId assignment = arena.addId(MultiAssignStmt());
        match(TK_LT);
        do {
            arena.node(assignment).names.push_back(peek().text);
            ++pos;
        } while (match(TK_COMMA));
        match(TK_GT);
        match(TK_COLON);
        const std::string functionName = peek().text;
        ++pos;
        if (!expect(TK_LPAREN, "'(' after function name")) {

            return 0;
        }
        arena.node(assignment).call = arena.add(CallExpr(functionName));
        if (peek().kind != TK_RPAREN) {
            do {
                AstRef<Expr> argument = parseExpression();
                if (!argument) {

                    return 0;
                }
                arena.node(assignment).call->args.push_back(argument);
            } while (match(TK_COMMA));
        }
        if (!expect(TK_RPAREN, "')' after function arguments")) {

            return 0;
        }
        arena.node(assignment).sourceLine = token.line;
        return AstRef<MultiAssignStmt>(arena, assignment.pool(), assignment.index());
    }

    if (token.kind == TK_PRINT || token.kind == TK_PRINTLN) {
        if (pos > 0 && tokens[pos - 1].line == token.line) {
            error = "line " + numberToString(token.line) + ": print command must start its line";
            return 0;
        }
        ++pos;
        PrintStmtId printId = arena.addId(PrintStmt(token.kind == TK_PRINTLN));
        while (peek().kind != TK_RBRACE && peek().kind != TK_EOF &&
               peek().line == token.line) {
            AstRef<Expr> item = parseExpression();
            if (!item) {

                return 0;
            }
            arena.node(printId).items.push_back(item);
            match(TK_COMMA); // Legacy comma-separated print lists remain accepted.

        }
        arena.node(printId).sourceLine = token.line;
        return AstRef<PrintStmt>(arena, printId.pool(), printId.index());
    }

    if (token.kind == TK_KEYWORD && token.text == "if") {
        ++pos;
        AstRef<Expr> condition = parseExpression();
        if (!condition) return 0;
        skipLines();
        AstRef<BlockStmt> yes = parseBlock();
        if (!yes) { return 0; }
        skipLines();
        AstRef<BlockStmt> no = 0;
        if (peek().kind == TK_KEYWORD && peek().text == "else") {
            ++pos;
            skipLines();
            if (peek().kind == TK_KEYWORD && peek().text == "if") {
                AstRef<Stmt> nested = parseStatement();
                if (!nested) return 0;
                no = arena.add(BlockStmt());
                no->statements.push_back(nested);
            } else {
                no = parseBlock();
                if (!no) { return 0; }
            }
        }
        AstRef<IfStmt> statement = arena.add(IfStmt(condition, yes, no));
        statement->sourceLine = token.line;
        return statement;
    }

    if (token.kind == TK_KEYWORD && token.text == "while") {
        ++pos;
        AstRef<Expr> condition = parseExpression();
        if (!condition) return 0;
        skipLines();
        AstRef<BlockStmt> body = parseBlock();
        if (!body) { return 0; }
        AstRef<WhileStmt> statement = arena.add(WhileStmt(condition, body));
        statement->sourceLine = token.line;
        return statement;
    }

    if (token.kind == TK_KEYWORD && token.text == "for") {
        ++pos;
        if (!expect(TK_IDENT, "loop variable")) return 0;
        const std::string variable = tokens[pos - 1].text;
        AstRef<Expr> start = 0;
        AstRef<Expr> limit = 0;
        AstRef<Expr> step = 0;
        if (match(TK_COLON)) {
            start = parseExpression();
            if (!start) return 0;
            if (peek().kind != TK_KEYWORD || peek().text != "to") {

                error = "line " + numberToString(peek().line) + ": expected 'to'";
                return 0;
            }
            ++pos;
            limit = parseExpression();
        } else if (match(TK_LT)) {
            limit = parseExpression();
        } else {
            error = "line " + numberToString(peek().line) +
                    ": expected ':' or '<' after loop variable";
            return 0;
        }
        if (!limit) { return 0; }
        if (peek().kind == TK_KEYWORD && peek().text == "step") {
            ++pos;
            step = parseExpression();
            if (!step) { return 0; }
        }
        skipLines();
        AstRef<BlockStmt> body = parseBlock();
        if (!body) { return 0; }
        AstRef<ForStmt> statement = arena.add(ForStmt(variable, limit, start, step, body));
        statement->sourceLine = token.line;
        return statement;
    }

    if (token.kind == TK_KEYWORD &&
        (token.text == "return" || token.text == "break" || token.text == "continue")) {
        ++pos;
        if (token.text != "return") {
            AstRef<SimpleStmt> statement = arena.add(SimpleStmt(token.text));
            statement->sourceLine = token.line;
            return statement;
        }
        ReturnStmtId resultId = arena.addId(ReturnStmt());
        if (!atStatementEnd()) {
            if (match(TK_LPAREN)) {
                arena.node(resultId).parenthesized = true;
                if (peek().kind != TK_RPAREN) {
                    do {
                        AstRef<Expr> value = parseExpression();
                        if (!value) { return 0; }
                        arena.node(resultId).values.push_back(value);
                    } while (match(TK_COMMA));
                }
                if (!expect(TK_RPAREN, "')' after return values")) {

                    return 0;
                }
            } else {
                AstRef<Expr> value = parseExpression();
                if (!value) { return 0; }
                arena.node(resultId).values.push_back(value);
            }
        }
        arena.node(resultId).sourceLine = token.line;
        return AstRef<ReturnStmt>(arena, resultId.pool(), resultId.index());
    }

    if (token.kind == TK_IDENT) {
        AstRef<Expr> target = arena.add(NameExpr(token.text));
        ++pos;
        while (match(TK_LBRACKET)) {
            AstRef<Expr> index = parseExpression();
            if (!index || !expect(TK_RBRACKET, "']'")) {

                return 0;
            }
            target = arena.add(IndexExpr(target, index));
        }
        std::string operation;
        if (match(TK_COLON) || match(TK_EQ)) operation = ":";
        else if (match(TK_PLUSCOLON)) operation = "+:";
        else if (match(TK_MINUSCOLON)) operation = "-:";
        else if (target && target.pool() == 1 && match(TK_LPAREN)) {
            if (target.arena() != &arena) throw std::logic_error("foreign parser expression");
            NameExprId nameId = arena.asNameExpr(ExprId(target.pool(), target.index(), arena.arenaIdentity()));
            AstRef<CallExpr> call = arena.add(CallExpr(arena.node(nameId).name));

            if (peek().kind != TK_RPAREN) {
                do {
                    AstRef<Expr> argument = parseExpression();
                    if (!argument) { return 0; }
                    call->args.push_back(argument);
                } while (match(TK_COMMA));
            }
            if (!expect(TK_RPAREN, "')'")) { return 0; }
            AstRef<ExprStmt> statement = arena.add(ExprStmt(call));
            statement->sourceLine = token.line;
            return statement;
        } else {

            error = "line " + numberToString(token.line) +
                    ": expected assignment or function call";
            return 0;
        }
        AstRef<Expr> value = parseInitializer();
        if (!value) { return 0; }
        AstRef<AssignStmt> statement = arena.add(AssignStmt(target, operation, value));
        statement->sourceLine = token.line;
        return statement;
    }

    error = "line " + numberToString(token.line) +
            ": unsupported statement beginning '" + token.text + "'";
    return 0;
}

AstRef<FunctionDef> Parser::parseFunction() {
    FunctionDefId functionId = arena.addId(FunctionDef());
    arena.node(functionId).sourceLine = peek().line;
    if (!expect(TK_KEYWORD, "'fun'")) { return 0; }

    if (peek().kind == TK_IDENT && peek(1).kind == TK_LPAREN) {
        arena.node(functionId).name = peek().text;
        ++pos;
    } else {
        if (!expect(TK_LT, "'<' starting output list")) { return 0; }
        if (peek().kind != TK_GT && peek().kind != TK_GE) {
            do {
                if (!isTypeToken(peek())) {
                    error = "line " + numberToString(peek().line) +
                            ": expected output type (int, real, char)";

                    return 0;
                }
                std::string outputType = peek().text;
                ++pos;
                if (match(TK_LBRACKET)) {
                    error = "line " + numberToString(peek().line) +
                            ": arrays, including char[], cannot be function return types; use a reference parameter";
                    return 0;
                }
                arena.node(functionId).returnTypes.push_back(outputType);
            } while (match(TK_COMMA));
        }
        const bool combinedSeparator = match(TK_GE);
        if (!combinedSeparator && !expect(TK_GT, "'>' after output list")) {

            return 0;
        }
        if (!combinedSeparator && !match(TK_EQ)) {
            error = "line " + numberToString(peek().line) +
                    ": expected '=' after output list";

            return 0;
        }
        if (!expect(TK_IDENT, "function name")) { return 0; }
        arena.node(functionId).name = tokens[pos - 1].text;
    }

    if (!expect(TK_LPAREN, "'(' after function name")) { return 0; }
    if (peek().kind != TK_RPAREN) {
        do {
            const bool byReference = match(TK_AND);
            if (!expect(TK_IDENT, "parameter name")) { return 0; }
            const std::string name = tokens[pos - 1].text;
            int rank = 0;
            while (match(TK_LBRACKET)) {
                if (!expect(TK_RBRACKET, "']' in array parameter")) {

                    return 0;
                }
                ++rank;
            }
            if (!expect(TK_COLON, "':' after parameter name")) { return 0; }
            if (!isTypeToken(peek())) {
                error = "line " + numberToString(peek().line) + ": expected parameter type";

                return 0;
            }
            arena.node(functionId).parameters.push_back(Parameter(peek().text, name, byReference, rank));
            ++pos;
        } while (match(TK_COMMA));
    }
    if (!expect(TK_RPAREN, "')' after parameters")) { return 0; }
    if (arena.node(functionId).name == "main" && !arena.node(functionId).parameters.empty()) {
        error = "main() must not have parameters";

        return 0;
    }
    if (arena.node(functionId).name == "main" && !arena.node(functionId).returnTypes.empty()) {
        error = "main() must not declare return values";

        return 0;
    }
    skipLines();
    arena.node(functionId).body = parseBlock();
    if (!arena.node(functionId).body) { return 0; }
    return AstRef<FunctionDef>(arena, functionId.pool(), functionId.index());
}

bool Parser::parse(const std::vector<Token>& input, Program& program, std::string& outputError) {
    tokens.clear();
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i].kind != TK_NEWLINE) tokens.push_back(input[i]);
    }
    if (tokens.empty() || tokens.back().kind != TK_EOF) {
        outputError = "token stream must end with EOF";
        return false;
    }
    pos = 0;
    error.clear();

    while (peek().kind != TK_EOF) {
        skipLines();
        if (peek().kind == TK_EOF) break;
        if (peek().kind == TK_KEYWORD && peek().text == "uses") {
            const int usesLine = peek().line;
            ++pos;
            if (peek().kind != TK_IDENT) {
                outputError = "line " + numberToString(usesLine) +
                              ": expected a borrowable library function after 'uses'";
                return false;
            }
            do {
                if (peek().kind != TK_IDENT) {
                    outputError = "line " + numberToString(peek().line) +
                                  ": expected library function name";
                    return false;
                }
                program.libraryUses.push_back(LibraryUse(peek().text, peek().line));
                ++pos;
                if (peek().kind != TK_COMMA) break;
                ++pos;
            } while (true);
            if (peek().kind != TK_EOF && peek().line == usesLine) {
                outputError = "line " + numberToString(peek().line) +
                              ": expected end of 'uses' declaration";
                return false;
            }
            skipLines();
            continue;
        }
        if (isTypeToken(peek())) {
            AstRef<Stmt> statement = parseStatement();
            AstRef<DeclStmt> declaration;
            if (statement && statement.pool() == 9) {
                if (statement.arena() != &arena) throw std::logic_error("foreign parser statement");
                DeclStmtId declarationId = arena.asDeclStmt(StmtId(statement.pool(), statement.index(), arena.arenaIdentity()));
                arena.node(declarationId);
                declaration = AstRef<DeclStmt>(arena, declarationId.pool(), declarationId.index());
            }
            if (!declaration) {

                outputError = "line " + numberToString(peek().line) +
                              ": expected global declaration";
                return false;
            }
            for (size_t i = 0; i < program.globals.size(); ++i) {
                if (program.globals[i]->name == declaration->name) {
                    outputError = "duplicate global '" + declaration->name + "'";

                    return false;
                }
            }
            program.globals.push_back(declaration);
        } else if (peek().kind == TK_KEYWORD && peek().text == "fun") {
            AstRef<FunctionDef> function = parseFunction();
            if (!function) { outputError = error; return false; }
            if (program.findFunctionId(function->name).valid()) {
                outputError = "duplicate function '" + function->name + "'";

                return false;
            }
            program.functions.push_back(function);
        } else {
            outputError = "line " + numberToString(peek().line) +
                          ": only global declarations and function definitions are allowed";
            return false;
        }
        skipLines();
    }

    if (!program.findFunctionId("main").valid()) {
        outputError = "no main() function found";
        return false;
    }
    outputError.clear();
    return true;
}

} // namespace shalimar
