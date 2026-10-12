#ifndef SHALIMAR_TOKEN_H
#define SHALIMAR_TOKEN_H

#include <string>

namespace shalimar {

enum TokenKind {
    TK_EOF,
    TK_NEWLINE,
    TK_IDENT,
    TK_INT,
    TK_REAL,
    TK_STRING,
    TK_LBRACE,
    TK_RBRACE,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_COMMA,
    TK_DOT,
    TK_COLON,
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_CARET,
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_GT,
    TK_LE,
    TK_GE,
    TK_AND,
    TK_OR,
    TK_PLUSCOLON,
    TK_MINUSCOLON,
    TK_PRINT,
    TK_PRINTLN,
    TK_KEYWORD
};

struct Token {
    TokenKind kind;
    std::string text;
    int line;
    int column;

    Token() : kind(TK_EOF), line(1), column(1) {}
    Token(TokenKind tokenKind, const std::string& tokenText, int tokenLine, int tokenColumn)
        : kind(tokenKind), text(tokenText), line(tokenLine), column(tokenColumn) {}
};

} // namespace shalimar

#endif // SHALIMAR_TOKEN_H
