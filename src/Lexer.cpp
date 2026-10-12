#include "Lexer.h"

#include <sstream>

namespace shalimar {

namespace {

std::string numberToString(int value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

bool isIdentifierStart(char character) {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') || character == '_';
}

bool isIdentifierPart(char character) {
    return isIdentifierStart(character) ||
           (character >= '0' && character <= '9');
}

std::string lowercase(const std::string& value) {
    std::string result = value;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = static_cast<char>(result[i] - 'A' + 'a');
        }
    }
    return result;
}

bool isKeyword(const std::string& value) {
    return value == "int" || value == "real" || value == "char" ||
           value == "fun" || value == "return" || value == "if" ||
           value == "else" || value == "while" || value == "for" ||
           value == "to" || value == "step" || value == "break" ||
           value == "continue" || value == "uses";
}

} // namespace

bool Lexer::tokenize(const std::string& source, std::vector<Token>& tokens,
                     std::string& error) {
    size_t index = 0;
    int line = 1;
    int column = 1;
    tokens.clear();
    error.clear();

    while (index < source.size()) {
        const char character = source[index];
        const int startLine = line;
        const int startColumn = column;

        if (character == ' ' || character == '\t' || character == '\r') {
            ++index;
            ++column;
            continue;
        }
        if (character == '\n') {
            tokens.push_back(Token(TK_NEWLINE, "\\n", line, column));
            ++index;
            ++line;
            column = 1;
            continue;
        }
        if (character == '/' && index + 1 < source.size() && source[index + 1] == '/') {
            while (index < source.size() && source[index] != '\n') {
                ++index;
                ++column;
            }
            continue;
        }

        if (isIdentifierStart(character)) {
            const size_t begin = index;
            while (index < source.size() && isIdentifierPart(source[index])) {
                ++index;
                ++column;
            }
            const std::string text = source.substr(begin, index - begin);
            const std::string normalized = lowercase(text);
            const TokenKind kind = isKeyword(normalized) ? TK_KEYWORD : TK_IDENT;
            tokens.push_back(Token(kind, text, startLine, startColumn));
            continue;
        }

        if (character >= '0' && character <= '9') {
            const size_t begin = index;
            bool isReal = false;
            while (index < source.size() && source[index] >= '0' && source[index] <= '9') {
                ++index;
                ++column;
            }
            if (index < source.size() && source[index] == '.') {
                isReal = true;
                ++index;
                ++column;
                while (index < source.size() && source[index] >= '0' && source[index] <= '9') {
                    ++index;
                    ++column;
                }
            }
            if (index < source.size() && (source[index] == 'e' || source[index] == 'E')) {
                isReal = true;
                ++index;
                ++column;
                if (index < source.size() && (source[index] == '+' || source[index] == '-')) {
                    ++index;
                    ++column;
                }
                const size_t exponentStart = index;
                while (index < source.size() && source[index] >= '0' && source[index] <= '9') {
                    ++index;
                    ++column;
                }
                if (exponentStart == index) {
                    error = "line " + numberToString(line) + ": malformed exponent";
                    return false;
                }
            }
            tokens.push_back(Token(isReal ? TK_REAL : TK_INT,
                                   source.substr(begin, index - begin),
                                   startLine, startColumn));
            continue;
        }

        if (character == '"') {
            const size_t begin = ++index;
            ++column;
            while (index < source.size() && source[index] != '"' && source[index] != '\n') {
                ++index;
                ++column;
            }
            if (index >= source.size() || source[index] != '"') {
                error = "line " + numberToString(startLine) + ": unterminated string literal";
                return false;
            }
            const std::string text = source.substr(begin, index - begin);
            ++index;
            ++column;
            tokens.push_back(Token(TK_STRING, text, startLine, startColumn));
            continue;
        }

        TokenKind kind = TK_EOF;
        size_t length = 1;
        if (character == '?' && index + 1 < source.size() && source[index + 1] == '?') {
            kind = TK_PRINT;
            ++length;
        } else if (character == '?') {
            kind = TK_PRINTLN;
        } else if (character == '+' && index + 1 < source.size() && source[index + 1] == ':') {
            kind = TK_PLUSCOLON;
            ++length;
        } else if (character == '-' && index + 1 < source.size() && source[index + 1] == ':') {
            kind = TK_MINUSCOLON;
            ++length;
        } else if (character == '!' && index + 1 < source.size() && source[index + 1] == '=') {
            kind = TK_NE;
            ++length;
        } else if (character == '<' && index + 1 < source.size() && source[index + 1] == '=') {
            kind = TK_LE;
            ++length;
        } else if (character == '>' && index + 1 < source.size() && source[index + 1] == '=') {
            kind = TK_GE;
            ++length;
        } else {
            switch (character) {
            case '{': kind = TK_LBRACE; break;
            case '}': kind = TK_RBRACE; break;
            case '(': kind = TK_LPAREN; break;
            case ')': kind = TK_RPAREN; break;
            case '[': kind = TK_LBRACKET; break;
            case ']': kind = TK_RBRACKET; break;
            case ',': kind = TK_COMMA; break;
            case '.': kind = TK_DOT; break;
            case ':': kind = TK_COLON; break;
            case '+': kind = TK_PLUS; break;
            case '-': kind = TK_MINUS; break;
            case '*': kind = TK_STAR; break;
            case '/': kind = TK_SLASH; break;
            case '%': kind = TK_PERCENT; break;
            case '^': kind = TK_CARET; break;
            case '=': kind = TK_EQ; break;
            case '<': kind = TK_LT; break;
            case '>': kind = TK_GT; break;
            case '&': kind = TK_AND; break;
            case '|': kind = TK_OR; break;
            default:
                error = "line " + numberToString(startLine) + ", column " +
                        numberToString(startColumn) + ": unsupported character";
                return false;
            }
        }
        tokens.push_back(Token(kind, source.substr(index, length), startLine, startColumn));
        index += length;
        column += static_cast<int>(length);
    }

    tokens.push_back(Token(TK_EOF, "", line, column));
    return true;
}

} // namespace shalimar
