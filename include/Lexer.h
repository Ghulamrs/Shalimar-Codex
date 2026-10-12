#ifndef SHALIMAR_LEXER_H
#define SHALIMAR_LEXER_H

#include "Token.h"

#include <string>
#include <vector>

namespace shalimar {

class Lexer {
public:
    bool tokenize(const std::string& source, std::vector<Token>& tokens,
                  std::string& error);
};

} // namespace shalimar

#endif // SHALIMAR_LEXER_H
