#ifndef SHALIMAR_SEMANTIC_CHECKER_H
#define SHALIMAR_SEMANTIC_CHECKER_H

#include "Ast.h"

#include <string>

namespace shalimar {

class SemanticChecker {
public:
    bool check(const Program& program, std::string& error);
};

} // namespace shalimar

#endif // SHALIMAR_SEMANTIC_CHECKER_H
