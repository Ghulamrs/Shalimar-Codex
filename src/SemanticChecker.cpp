#include "SemanticChecker.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <cstdlib>

namespace shalimar {

namespace {

struct FunctionInfo {
    std::vector<std::string> outputs;
    std::vector<Parameter> parameters;
};

struct GlobalInfo {
    std::string type;
    int sourceLine;
};

bool statementReturns(AstRef<const Stmt> statement);

// Function calls (including conversion syntax) are forbidden inside any
// parenthesized multiple-return argument, at any nesting depth.
bool containsReturnCall(AstRef<const Expr> expression) {
    if (!expression) return false;
    if (astCast<const CallExpr>(expression)) return true;
    AstRef<const UnaryExpr> unary = astCast<const UnaryExpr>(expression);
    if (unary) return containsReturnCall(unary->value);
    AstRef<const BinaryExpr> binary = astCast<const BinaryExpr>(expression);
    if (binary) return containsReturnCall(binary->left) ||
                       containsReturnCall(binary->right);
    AstRef<const IndexExpr> index = astCast<const IndexExpr>(expression);
    if (index) return containsReturnCall(index->base) ||
                      containsReturnCall(index->index);
    AstRef<const AttributeExpr> attribute = astCast<const AttributeExpr>(expression);
    if (attribute) return containsReturnCall(attribute->base) ||
                          containsReturnCall(attribute->axis);
    AstRef<const ArrayLiteralExpr> array = astCast<const ArrayLiteralExpr>(expression);
    if (array) {
        for (size_t i = 0; i < array->values.size(); ++i)
            if (containsReturnCall(array->values[i])) return true;
    }
    return false;
}


std::string numberToString(int value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

bool blockReturns(AstRef<const BlockStmt> block) {
    for (size_t i = 0; i < block->statements.size(); ++i) {
        if (statementReturns(block->statements[i])) return true;
    }
    return false;
}

bool statementReturns(AstRef<const Stmt> statement) {
    // Transitional P0 migration: use checked reference narrowing without
    // introducing another custom AST template cast or raw owning pointer.
    if (!statement) return false;
    try {
        (void)dynamic_cast<const ReturnStmt&>(statement.value());
        return true;
    } catch (const std::bad_cast&) {
        // Not a return statement; an if/else may still return on both paths.
    }
    try {
        const IfStmt& conditional = dynamic_cast<const IfStmt&>(statement.value());
        return conditional.no && blockReturns(conditional.yes) &&
               blockReturns(conditional.no);
    } catch (const std::bad_cast&) {
        return false;
    }
}

std::string arrayType(const std::string& type, int rank) {
    std::string result = type;
    for (int i = 0; i < rank; ++i) result += "[]";
    return result;
}

bool librarySignature(const std::string& name, int& minimumArity,
                      int& maximumArity, std::string& reason) {
    minimumArity = maximumArity = 1;
    reason.clear();
    if (name == "abs" || name == "sqrt" || name == "log" || name == "exp" ||
        name == "sin" || name == "cos" || name == "tan" || name == "asin" ||
        name == "acos" || name == "atan" || name == "round" || name == "ceil" ||
        name == "floor" || name == "trunc" || name == "log10" || name == "log2" ||
        name == "cbrt" || name == "sinh" || name == "cosh" || name == "tanh") {
        return true;
    }
    if (name == "hypot" || name == "atan2" || name == "pow" || name == "fmod" ||
        name == "max" || name == "min") {
        minimumArity = maximumArity = 2;
        return true;
    }
    if (name == "len") return true;
    if (name == "memset" || name == "memcpy" || name == "strcpy" ||
        name == "printf" || name == "scanf") {
        reason = name == "printf" || name == "scanf"
            ? "variadic or pointer-based signatures are not representable in Shalimar"
            : "pointer parameters are not representable in Shalimar";
        return false;
    }
    reason = "the name is not in Shalimar's borrowable library-function table";
    return false;
}

bool isNumericType(const std::string& type) {
    return type == "int" || type == "real" || type == "char";
}

} // namespace

class CheckerImpl {
public:
    explicit CheckerImpl(const Program& source)
        : program(source), currentFunction(0), currentSourceLine(0), loopDepth(0) {}

    bool run(std::string& outputError) {
        for (size_t i = 0; i < program.libraryUses.size(); ++i) {
            const LibraryUse& use = program.libraryUses[i];
            int minimumArity = 0;
            int maximumArity = 0;
            std::string reason;
            if (!librarySignature(use.name, minimumArity, maximumArity, reason)) {
                outputError = "line " + numberToString(use.sourceLine) +
                              ": cannot borrow '" + use.name + "': " + reason;
                return false;
            }
            if (borrowedFunctions.find(use.name) == borrowedFunctions.end()) {
                borrowedFunctions[use.name] = use.sourceLine;
            }
        }

        for (size_t i = 0; i < program.functions.size(); ++i) {
            AstRef<const FunctionDef> function = program.functions[i];
            FunctionInfo info;
            info.outputs = function->returnTypes;
            info.parameters = function->parameters;
            functions[function->name] = info;
        }

        for (size_t i = 0; i < program.globals.size(); ++i) {
            AstRef<const DeclStmt> global = program.globals[i];
            if (borrowedFunctions.find(global->name) != borrowedFunctions.end()) {
                outputError = "global '" + global->name + "' is borrowed on line " +
                              numberToString(borrowedFunctions[global->name]) +
                              " - drop the borrow or use another name";
                return false;
            }
            GlobalInfo info;
            info.type = arrayType(global->type,
                                  static_cast<int>(global->dimensions.size()));
            info.sourceLine = global->sourceLine;
            globalTypes[global->name] = info;
            if (!global->dimensions.empty()) {
                std::vector<long> extents;
                for (size_t dimension = 0;
                     dimension < global->dimensions.size(); ++dimension) {
                    long extent = -1;
                    constantInteger(global->dimensions[dimension], extent);
                    extents.push_back(extent);
                }
                declaredArrayDimensions[global->name] = extents;
            }
        }

        std::map<std::string, std::string> visibleGlobals;
        for (size_t i = 0; i < program.globals.size(); ++i) {
            AstRef<const DeclStmt> global = program.globals[i];
            currentFunction = 0;
            currentSourceLine = global->sourceLine;
            scopes.clear();
            scopes.push_back(visibleGlobals);
            error.clear();

            bool valid = true;
            for (size_t dimension = 0; dimension < global->dimensions.size(); ++dimension) {
                if (expressionType(global->dimensions[dimension]) != "int") {
                    valid = fail("global array dimensions must be int expressions");
                    break;
                }
                long extent = 0;
                if (!constantInteger(global->dimensions[dimension], extent)) {
                    valid = fail("global array dimensions must be compile-time integer constants");
                    break;
                }
                if (extent < 1) {
                    valid = fail("global array dimensions must be at least 1");
                    break;
                }
            }
            if (valid && global->init) {
                if (global->dimensions.empty()) {
                    const std::string initType = expressionType(global->init);
                    if (!assignable(global->type, initType)) {
                        valid = fail("cannot initialize global " + global->type +
                                     " variable '" + global->name + "' with " + initType);
                    }
                } else {
                    AstRef<const LiteralExpr> stringInitializer =
                        astCast<const LiteralExpr>(global->init);
                    const bool charTextInitializer = global->type == "char" &&
                        global->dimensions.size() == 1 &&
                        expressionType(global->init) == "char[]";
                    if (charTextInitializer) {
                        long extent = 0;
                        if (!constantInteger(global->dimensions[0], extent) || extent < 1) {
                            valid = fail("global char[] string initialization requires a positive constant capacity");
                        } else if (stringInitializer && !stringInitializer->stringValue) {
                            valid = fail("global char[] string initialization requires a string expression");
                        }
                    } else {
                        AstRef<const ArrayLiteralExpr> literal =
                            astCast<const ArrayLiteralExpr>(global->init);
                        if (!literal) {
                            valid = fail("global array initializer must be an array literal or char[] string");
                        } else {
                            valid = checkArrayLiteral(literal, global, 0);
                        }
                    }
                }
            }
            if (!error.empty()) {
                outputError = "global '" + global->name + "' on line " +
                              numberToString(global->sourceLine) + ": " + error;
                return false;
            }
            visibleGlobals[global->name] = arrayType(global->type, static_cast<int>(global->dimensions.size()));
        }

        for (size_t i = 0; i < program.functions.size(); ++i) {
            AstRef<const FunctionDef> function = program.functions[i];
            currentFunction = function;
            currentSourceLine = function->sourceLine;
            error.clear();
            scopes.clear();
            scopes.push_back(std::map<std::string, std::string>());
            for (std::map<std::string, GlobalInfo>::const_iterator global =
                     globalTypes.begin(); global != globalTypes.end(); ++global) {
                if (global->second.sourceLine < function->sourceLine) {
                    scopes.back()[global->first] = global->second.type;
                }
            }
            scopes.push_back(std::map<std::string, std::string>());
            for (size_t j = 0; j < function->parameters.size(); ++j) {
                const Parameter& parameter = function->parameters[j];
                if (borrowedFunctions.find(parameter.name) != borrowedFunctions.end()) {
                    fail("parameter '" + parameter.name + "' is borrowed on line " +
                         numberToString(borrowedFunctions[parameter.name]) +
                         " - drop the borrow or use another name");
                }
                if (scopes.back().find(parameter.name) != scopes.back().end()) {
                    fail("duplicate parameter '" + parameter.name + "'");
                }
                scopes.back()[parameter.name] = arrayType(parameter.type, parameter.arrayRank);
            }
            checkBlock(function->body);
            for (size_t j = 0; j < function->parameters.size(); ++j) {
                if (function->parameters[j].byReference &&
                    function->parameters[j].arrayRank != 0) {
                    fail("'&' is only valid for scalar parameters; arrays are already passed by reference");
                }
            }
            if (!function->returnTypes.empty() && !blockReturns(function->body)) {
                fail("value-returning function must return on every path");
            }
            if (!error.empty()) {
                outputError = "in function '" + function->name + "': " + error;
                return false;
            }
        }
        outputError.clear();
        return true;
    }

private:
    const Program& program;
    std::map<std::string, FunctionInfo> functions;
    std::map<std::string, GlobalInfo> globalTypes;
    std::map<std::string, int> borrowedFunctions;
    std::map<std::string, std::vector<long> > declaredArrayDimensions;
    std::vector<std::map<std::string, std::string> > scopes;
    AstRef<const FunctionDef> currentFunction;
    int currentSourceLine;
    std::string error;
    int loopDepth;

    bool fail(const std::string& message) {
        if (error.empty()) error = "line " + numberToString(currentSourceLine) + ": " + message;
        return false;
    }

    std::string lookup(const std::string& name) const {
        for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
            std::map<std::string, std::string>::const_iterator found =
                scopes[i].find(name);
            if (found != scopes[i].end()) return found->second;
        }
        return "";
    }

    bool numeric(const std::string& type) const {
        return type == "int" || type == "real" || type == "char";
    }

    bool addressable(AstRef<const Expr> expression) const {
        if (!expression) return false;
        const Expr& node = expression.value();
        if (dynamic_cast<const NameExpr*>(&node) != 0) return true;
        const IndexExpr* index = dynamic_cast<const IndexExpr*>(&node);
        return index != 0 && addressable(index->base);
    }

    bool validArgument(const Parameter& parameter, AstRef<const Expr> argument,
                       const std::string& actualType) {
        const std::string expected = arrayType(parameter.type, parameter.arrayRank);
        if (parameter.byReference) {
            if (!addressable(argument)) {
                return fail("reference argument must be a variable or array element");
            }
            if (actualType != expected) {
                return fail("reference argument type must exactly match parameter type");
            }
            if (parameter.arrayRank != 0) {
                return fail("'&' is only valid for scalar parameters");
            }
            return true;
        }
        if (!assignable(expected, actualType)) {
            return fail("argument type mismatch");
        }
        return true;
    }

    bool assignable(const std::string& destination, const std::string& source) const {
        if (destination == source) return true;
        if (destination.find("[]") != std::string::npos ||
            source.find("[]") != std::string::npos) return false;
        return (destination == "real" && source == "int") ||
               (destination == "int" && (source == "real" || source == "char")) ||
               (destination == "char" && (source == "int" || source == "real"));
    }

    std::string expressionType(AstRef<const Expr> expression) {
        if (!expression) return "";
        AstRef<const LiteralExpr> literal = astCast<const LiteralExpr>(expression);
        if (literal) {
            if (literal->stringValue) return "char[]";
            return literal->value.find_first_of(".eE") != std::string::npos ? "real" : "int";
        }
        AstRef<const NameExpr> name = astCast<const NameExpr>(expression);
        if (name) {
            const std::string type = lookup(name->name);
            name->constantValue.clear();
            if (type.empty() && (name->name == "pi" || name->name == "e")) {
                name->constantValue = name->name == "pi" ? "3.14159265358979323846" : "2.71828182845904523536";
                return "real";
            }
            if (type.empty()) {
                std::map<std::string, GlobalInfo>::const_iterator global =
                    globalTypes.find(name->name);
                if (global != globalTypes.end() &&
                    global->second.sourceLine >= currentSourceLine) {
                    fail("'" + name->name + "' is a global declared later, on line " +
                         numberToString(global->second.sourceLine));
                } else {
                    fail("unknown variable '" + name->name + "'");
                }
            }
            return type;
        }
        AstRef<const IndexExpr> index = astCast<const IndexExpr>(expression);
        if (index) {
            const std::string baseType = expressionType(index->base);
            const std::string indexType = expressionType(index->index);
            if (indexType != "int") fail("array index must be int");
            const size_t suffix = baseType.find("[]");
            if (suffix == std::string::npos) {
                fail("indexing requires an array");
                return "";
            }
            return baseType.substr(0, suffix) + baseType.substr(suffix + 2);
        }
        AstRef<const AttributeExpr> attribute = astCast<const AttributeExpr>(expression);
        if (attribute) {
            const std::string baseType = expressionType(attribute->base);
            const size_t suffix = baseType.find("[]");
            if (suffix == std::string::npos) {
                fail("array attribute ." + attribute->attribute + " requires an array");
                return "";
            }
            if (attribute->axis && expressionType(attribute->axis) != "int") {
                fail(".dim axis must be numeric");
            }
            return "int";
        }
        AstRef<const ArrayLiteralExpr> array = astCast<const ArrayLiteralExpr>(expression);
        if (array) {
            std::string elementType;
            for (size_t i = 0; i < array->values.size(); ++i) {
                if (!array->values[i]) continue;
                const std::string itemType = expressionType(array->values[i]);
                if (itemType.find("[]") != std::string::npos) {
                    fail("nested array literals are not supported by this compiler stage");
                } else if (elementType.empty()) {
                    elementType = itemType;
                } else if (!assignable(elementType, itemType) &&
                           !assignable(itemType, elementType)) {
                    fail("array literal elements must have compatible types");
                } else if (elementType == "int" && itemType == "real") {
                    elementType = "real";
                }
            }
            return arrayType(elementType.empty() ? "int" : elementType, 1);
        }
        AstRef<const UnaryExpr> unary = astCast<const UnaryExpr>(expression);
        if (unary) {
            const std::string type = expressionType(unary->value);
            if (!numeric(type)) fail("unary '-' requires a numeric operand");
            return type;
        }
        AstRef<const BinaryExpr> binary = astCast<const BinaryExpr>(expression);
        if (binary) {
            const std::string left = expressionType(binary->left);
            const std::string right = expressionType(binary->right);
            if (binary->op == "+" && left == "char[]" && right == "char[]") {
                return "char[]";
            }
            if (binary->op == "+" || binary->op == "-" || binary->op == "*" ||
                binary->op == "/" || binary->op == "%" || binary->op == "^") {
                if (!numeric(left) || !numeric(right)) {
                    fail("arithmetic operator requires numeric operands, or two strings for concatenation");
                }
                if (binary->op == "^") return "real";
                return left == "real" || right == "real" ? "real" : "int";
            }
            if (binary->op == "&" || binary->op == "|") {
                if (!numeric(left) || !numeric(right)) fail("logical operator requires numeric operands");
                return "int";
            }
            if (binary->op == "=" || binary->op == "!=" || binary->op == "<" ||
                binary->op == ">" || binary->op == "<=" || binary->op == ">=") {
                const bool stringComparison = left == "char[]" && right == "char[]";
                if (!stringComparison && (!numeric(left) || !numeric(right))) {
                    fail("comparison requires two numeric operands or two strings");
                }
                return "int";
            }
        }
        AstRef<const CallExpr> call = astCast<const CallExpr>(expression);
        if (call) {
            if (call->name == "int" || call->name == "real" || call->name == "char") {
                if (call->args.size() != 1) fail("conversion requires exactly one argument");
                else if (!numeric(expressionType(call->args[0]))) {
                    fail("conversion requires a numeric argument");
                }
                return call->name;
            }
            std::map<std::string, FunctionInfo>::const_iterator function = functions.find(call->name);
            if (function == functions.end()) {
                std::vector<std::string> argumentTypes;
                for (size_t i = 0; i < call->args.size(); ++i) {
                    argumentTypes.push_back(expressionType(call->args[i]));
                }
                std::map<std::string, int>::const_iterator borrowed =
                    borrowedFunctions.find(call->name);
                if (borrowed == borrowedFunctions.end()) {
                    int minimumArity = 0;
                    int maximumArity = 0;
                    std::string reason;
                    if (librarySignature(call->name, minimumArity, maximumArity, reason)) {
                        fail("library function '" + call->name +
                             "' must be borrowed with 'uses " + call->name + "'");
                    } else {
                        fail("call to unknown function '" + call->name + "'");
                    }
                    return "";
                }
                int minimumArity = 0;
                int maximumArity = 0;
                std::string reason;
                librarySignature(call->name, minimumArity, maximumArity, reason);
                if (call->args.size() < static_cast<size_t>(minimumArity) ||
                    call->args.size() > static_cast<size_t>(maximumArity)) {
                    fail("library function '" + call->name + "' takes " +
                         numberToString(minimumArity) + " argument(s), got " +
                         numberToString(static_cast<int>(call->args.size())));
                    return "";
                }
                if (call->name == "len") {
                    if (argumentTypes[0].find("[]") == std::string::npos) {
                        fail("'len' requires an array or string argument");
                        return "";
                    }
                    return "int";
                }
                for (size_t i = 0; i < argumentTypes.size(); ++i) {
                    if (!isNumericType(argumentTypes[i])) {
                        fail("library function '" + call->name + "' requires numeric arguments");
                        return "";
                    }
                }
                if (call->name == "abs" && argumentTypes[0] == "int") return "int";
                if ((call->name == "max" || call->name == "min") &&
                    argumentTypes[0] == "int" && argumentTypes[1] == "int") return "int";
                if (call->name == "abs") {
                    return argumentTypes[0] == "int" ? "int" : "real";
                }
                if (call->name == "max" || call->name == "min") {
                    return argumentTypes[0] == "int" && argumentTypes[1] == "int"
                        ? "int" : "real";
                }
                return "real";
            }
            const FunctionInfo& info = function->second;
            if (call->args.size() != info.parameters.size()) {
                fail("function '" + call->name + "' called with wrong number of arguments");
            }
            const size_t count = call->args.size() < info.parameters.size()
                ? call->args.size() : info.parameters.size();
            for (size_t i = 0; i < call->args.size(); ++i) {
                const std::string argumentType = expressionType(call->args[i]);
                if (i < count) {
                    const Parameter& parameter = info.parameters[i];
                    if (!validArgument(parameter, call->args[i], argumentType) && error.empty()) {
                        fail("invalid argument in call to '" + call->name + "'");
                    }
                }
            }
            if (info.outputs.size() != 1) {
                fail("function '" + call->name + "' does not return exactly one value in expression context");
                return "";
            }
            return info.outputs[0];
        }
        return "";
    }

    bool inferLiteral(AstRef<const ArrayLiteralExpr> literal,
                      std::string& type, std::vector<size_t>& dimensions,
                      size_t depth) {
        if (dimensions.size() <= depth) dimensions.push_back(0);
        if (dimensions[depth] < literal->values.size()) dimensions[depth] = literal->values.size();
        for (size_t i = 0; i < literal->values.size(); ++i) {
            if (!literal->values[i]) continue;
            AstRef<const ArrayLiteralExpr> nested = astCast<const ArrayLiteralExpr>(literal->values[i]);
            if (nested) {
                if (!inferLiteral(nested, type, dimensions, depth + 1)) return false;
            } else {
                const std::string item = expressionType(literal->values[i]);
                if (item != "int" && item != "real" && item != "char") return fail("invalid array literal element");
                if (type.empty()) type = item;
                else if ((type == "char") != (item == "char")) return fail("char and numeric elements cannot mix");
                else if (item == "real") type = "real";
            }
        }
        return error.empty();
    }

    bool checkBlock(AstRef<const BlockStmt> block) {
        // The handle resolves the arena-owned block by index, without owning pointers.
        BlockStmt& mutableBlock = dynamic_cast<BlockStmt&>(program.arena.resolve(block.pool(), block.index()));
        scopes.push_back(std::map<std::string, std::string>());
        bool valid = true;
        for (size_t i = 0; i < mutableBlock.statements.size(); ++i) {
            AstRef<Stmt> statement = mutableBlock.statements[i];
            currentSourceLine = statement->sourceLine;
            AstRef<AssignStmt> assignment = astCast<AssignStmt>(statement);
            AstRef<const NameExpr> target = assignment ? astCast<const NameExpr>(assignment->target) : AstRef<const NameExpr>();
            if (target && lookup(target->name).empty() && assignment->op == ":") {
                AstRef<const ArrayLiteralExpr> literal = astCast<const ArrayLiteralExpr>(assignment->value);
                std::string type;
                std::vector<size_t> dimensions;
                if (literal) {
                    if (!inferLiteral(literal, type, dimensions, 0)) { valid = false; break; }
                    if (type.empty()) { valid = fail("array literal needs at least one typed element"); break; }
                } else {
                    type = expressionType(assignment->value);
                    if (type.find("[]") != std::string::npos && type != "char[]") {
                        valid = fail("Declare the array '" + target->name + "' first"); break;
                    }
                }
                AstRef<DeclStmt> declaration = program.arena.add(DeclStmt(type == "char[]" ? "char" : type, target->name, statement->sourceLine));
                declaration->implicit = true;
                declaration->init = assignment->value;
                if (type == "char[]") {
                    declaration->inferredString = true;
                    declaration->dimensions.push_back(program.arena.add(LiteralExpr("1")));
                } else {
                    for (size_t d = 0; d < dimensions.size(); ++d) {
                        if (dimensions[d] == 0) { valid = fail("empty literal cannot establish an array extent"); break; }
                        declaration->dimensions.push_back(program.arena.add(LiteralExpr(numberToString(static_cast<int>(dimensions[d])))));
                    }
                }
                mutableBlock.statements[i] = declaration;
                statement = declaration;
            }
            AstRef<const MultiAssignStmt> multi = astCast<const MultiAssignStmt>(statement);
            if (multi) {
                std::map<std::string, FunctionInfo>::const_iterator function = functions.find(multi->call->name);
                if (function != functions.end() && multi->names.size() == function->second.outputs.size()) {
                    std::vector<AstRef<Stmt> > declarations;
                    for (size_t d = 0; d < multi->names.size(); ++d) {
                        if (lookup(multi->names[d]).empty()) {
                            AstRef<DeclStmt> declaration = program.arena.add(DeclStmt(function->second.outputs[d], multi->names[d], statement->sourceLine));
                            declaration->implicit = true;
                            if (!checkStatement(declaration)) valid = false;
                            declarations.push_back(declaration);
                        }
                    }
                    mutableBlock.statements.insert(mutableBlock.statements.begin() + i, declarations.begin(), declarations.end());
                    i += declarations.size();
                }
            }
            if (!checkStatement(statement)) valid = false;
            if (!valid) break;
        }
        scopes.pop_back();
        return valid;
    }

    bool constantInteger(AstRef<const Expr> expression, long& value) const {
        AstRef<const LiteralExpr> literal = astCast<const LiteralExpr>(expression);
        if (!literal || literal->stringValue ||
            literal->value.find_first_of(".eE") != std::string::npos) {
            return false;
        }
        char* end = 0;
        value = std::strtol(literal->value.c_str(), &end, 10);
        return end && *end == '\0';
    }

    bool checkArrayLiteral(AstRef<const ArrayLiteralExpr> literal,
                           AstRef<const DeclStmt> declaration,
                           size_t dimensionIndex) {
        if (dimensionIndex >= declaration->dimensions.size()) {
            return fail("array initializer has more dimensions than the declaration");
        }

        long extent = 0;
        const bool knownExtent = constantInteger(
            declaration->dimensions[dimensionIndex], extent);
        if (knownExtent && static_cast<long>(literal->values.size()) > extent) {
            return fail("array initializer has more entries than dimension " +
                        numberToString(static_cast<int>(dimensionIndex)) + " permits");
        }

        for (size_t i = 0; i < literal->values.size(); ++i) {
            AstRef<const Expr> item = literal->values[i];
            if (!item) continue; // omitted slots are zero-filled

            AstRef<const ArrayLiteralExpr> nested = astCast<const ArrayLiteralExpr>(item);
            const bool hasMoreDimensions =
                dimensionIndex + 1 < declaration->dimensions.size();
            if (hasMoreDimensions) {
                if (!nested) {
                    return fail("each row of a multidimensional array initializer must be an array literal");
                }
                if (!checkArrayLiteral(nested, declaration, dimensionIndex + 1)) {
                    return false;
                }
            } else {
                if (nested) {
                    return fail("array initializer has more dimensions than the declaration");
                }
                const std::string elementType = expressionType(item);
                if (!assignable(declaration->type, elementType)) {
                    return fail("array initializer element type mismatch");
                }
            }
        }
        return true;
    }

    bool checkStatement(AstRef<const Stmt> statement) {
        currentSourceLine = statement->sourceLine;
        AstRef<const DeclStmt> declaration = astCast<const DeclStmt>(statement);
        if (declaration) {
            if (borrowedFunctions.find(declaration->name) != borrowedFunctions.end()) {
                return fail("variable '" + declaration->name + "' is borrowed on line " +
                            numberToString(borrowedFunctions[declaration->name]) +
                            " - drop the borrow or use another name");
            }
            if (scopes.back().find(declaration->name) != scopes.back().end()) {
                return fail("duplicate declaration of '" + declaration->name + "'");
            }
            const std::string type = arrayType(declaration->type,
                static_cast<int>(declaration->dimensions.size()));
            scopes.back()[declaration->name] = type;
            if (!declaration->dimensions.empty()) {
                std::vector<long> extents;
                for (size_t dimension = 0; dimension < declaration->dimensions.size(); ++dimension) {
                    long extent = -1;
                    constantInteger(declaration->dimensions[dimension], extent);
                    extents.push_back(extent);
                }
                declaredArrayDimensions[declaration->name] = extents;
            }
            bool valid = true;
            if (declaration->type == "char" && declaration->dimensions.size() > 1) {
                valid = fail("char arrays must be one-dimensional");
            }
            for (size_t i = 0; i < declaration->dimensions.size(); ++i) {
                if (expressionType(declaration->dimensions[i]) != "int") {
                    valid = fail("array dimensions must be int expressions");
                }
                long extent = 0;
                if (constantInteger(declaration->dimensions[i], extent) && extent < 1) {
                    valid = fail("array dimensions must be at least 1");
                }
            }
            const bool charTextInitializer = !declaration->dimensions.empty() &&
                declaration->dimensions.size() == 1 && declaration->type == "char" &&
                expressionType(declaration->init) == "char[]";
            if (!declaration->dimensions.empty() && declaration->init &&
                !astCast<const ArrayLiteralExpr>(declaration->init) &&
                !charTextInitializer) {
                valid = fail("array initializer must be an array literal or char[] string");
            }
            if (declaration->init) {
                if (!declaration->dimensions.empty()) {
                    AstRef<const ArrayLiteralExpr> literal =
                        astCast<const ArrayLiteralExpr>(declaration->init);
                    if (charTextInitializer) {
                        long extent = 0;
                        if (!constantInteger(declaration->dimensions[0], extent)) {
                            valid = fail("runtime-sized char[] string initialization is not supported yet");
                        } else if (extent < 1) {
                            valid = fail("char[] capacity must be at least 1 for the terminator");
                        }
                    } else if (!literal) {
                        valid = fail("array initializer must be an array literal or char[] string");
                    } else {
                        valid = checkArrayLiteral(literal, declaration, 0);
                    }
                } else {
                    const std::string initType = expressionType(declaration->init);
                    if (!assignable(declaration->type, initType)) {
                        valid = fail("cannot initialize " + declaration->type + " variable '" +
                                     declaration->name + "' with " + initType);
                    }
                }
            }
            return valid;
        }

        AstRef<const AssignStmt> assignment = astCast<const AssignStmt>(statement);
        if (assignment) {
            const std::string destination = expressionType(assignment->target);
            AstRef<const ArrayLiteralExpr> arrayValue =
                astCast<const ArrayLiteralExpr>(assignment->value);
            const std::string source = arrayValue ? "" : expressionType(assignment->value);
            if (arrayValue) {
                AstRef<const NameExpr> arrayTarget = astCast<const NameExpr>(assignment->target);
                AstRef<const IndexExpr> rowTarget = astCast<const IndexExpr>(assignment->target);
                if (assignment->op != ":") {
                    fail("array literal assignment requires ':' assignment");
                } else if ((!arrayTarget && !rowTarget) || destination.find("[]") == std::string::npos) {
                    fail("array literal assignment target must be a declared array or array row");
                } else if (rowTarget) {
                    // A row is an array view. Validate each supplied element against
                    // the element type without treating the row as a scalar.
                    AstRef<DeclStmt> shape = program.arena.add(DeclStmt(
                        destination.substr(0, destination.find("[")), "__row_shape"));
                    shape->dimensions.push_back(program.arena.add(NameExpr("__runtime_row_extent")));
                    checkArrayLiteral(arrayValue, shape, 0);
                } else {
                    const Parameter* arrayParameter = 0;
                    if (currentFunction) {
                        for (size_t i = 0; i < currentFunction->parameters.size(); ++i) {
                            const Parameter& candidate = currentFunction->parameters[i];
                            if (candidate.name == arrayTarget->name && candidate.arrayRank > 0) {
                                arrayParameter = &candidate;
                                break;
                            }
                        }
                    }
                    if (arrayParameter) {
                        AstRef<DeclStmt> shape = program.arena.add(DeclStmt(arrayParameter->type, "__array_parameter_shape"));
                        for (int dimension = 0; dimension < arrayParameter->arrayRank; ++dimension) {
                            shape->dimensions.push_back(program.arena.add(NameExpr(
                                "__runtime_array_dimension" + numberToString(dimension))));
                        }
                        checkArrayLiteral(arrayValue, shape, 0);
                    } else {
                        std::map<std::string, std::vector<long> >::const_iterator extents =
                            declaredArrayDimensions.find(arrayTarget->name);
                        if (extents == declaredArrayDimensions.end()) {
                            fail("array literal assignment requires an array with known dimensions");
                        } else {
                            AstRef<DeclStmt> shape = program.arena.add(DeclStmt("int", "__array_shape"));
                            shape->type = destination.substr(0, destination.find("["));
                            for (size_t i = 0; i < extents->second.size(); ++i) {
                                AstRef<Expr> dimension = extents->second[i] < 0
                                    ? astCast<Expr>(program.arena.add(NameExpr(
                                          "__runtime_array_dimension" +
                                          numberToString(static_cast<int>(i)))))
                                    : astCast<Expr>(program.arena.add(LiteralExpr(numberToString(
                                          static_cast<int>(extents->second[i])))));
                                shape->dimensions.push_back(dimension);
                            }
                            if (extents->second.empty()) {
                                fail("array literal assignment target has no dimension metadata");
                            } else {
                                checkArrayLiteral(arrayValue, shape, 0);
                            }
                        }
                    }
                }
            } else if (assignment->op == "+:" && destination == "char[]" && source == "char[]") {
                // Bounded string append is supported by fixed-capacity char arrays.
            } else if (!assignable(destination, source)) {
                fail("assignment type mismatch");
            }
            if (assignment->op != ":" &&
                !(assignment->op == "+:" && destination == "char[]" && source == "char[]") &&
                !numeric(destination)) {
                fail("compound assignment requires a numeric scalar or char[] append target");
            }
            return error.empty();
        }

        AstRef<const MultiAssignStmt> multi = astCast<const MultiAssignStmt>(statement);
        if (multi) {
            std::map<std::string, FunctionInfo>::const_iterator function = functions.find(multi->call->name);
            if (function == functions.end()) {
                fail("call to unknown function '" + multi->call->name + "'");
                return false;
            }
            const FunctionInfo& info = function->second;
            if (multi->names.size() != info.outputs.size()) {
                fail("multi-assignment output count does not match function '" + multi->call->name + "'");
            }
            if (multi->call->args.size() != info.parameters.size()) {
                fail("function '" + multi->call->name + "' called with wrong number of arguments");
            }
            const size_t argumentCount = multi->call->args.size() < info.parameters.size()
                ? multi->call->args.size() : info.parameters.size();
            for (size_t i = 0; i < multi->call->args.size(); ++i) {
                const std::string argumentType = expressionType(multi->call->args[i]);
                if (i < argumentCount) {
                    const Parameter& parameter = info.parameters[i];
                    if (!validArgument(parameter, multi->call->args[i], argumentType) && error.empty()) {
                        fail("invalid argument in call to '" + multi->call->name + "'");
                    }
                }
            }
            const size_t count = multi->names.size() < info.outputs.size()
                ? multi->names.size() : info.outputs.size();
            for (size_t i = 0; i < count; ++i) {
                const std::string destination = lookup(multi->names[i]);
                if (destination.empty()) {
                    fail("multi-assignment to unknown variable '" + multi->names[i] + "'");
                } else if (!assignable(destination, info.outputs[i])) {
                    fail("multi-assignment type mismatch for '" + multi->names[i] + "'");
                }
            }
            return error.empty();
        }

        AstRef<const ExprStmt> expressionStatement = astCast<const ExprStmt>(statement);
        if (expressionStatement) {
            AstRef<const CallExpr> call = astCast<const CallExpr>(expressionStatement->value);
            if (call) {
                std::map<std::string, FunctionInfo>::const_iterator function =
                    functions.find(call->name);
                if (function != functions.end() && function->second.outputs.empty()) {
                    const FunctionInfo& info = function->second;
                    if (call->args.size() != info.parameters.size()) {
                        fail("function '" + call->name + "' called with wrong number of arguments");
                    }
                    const size_t count = call->args.size() < info.parameters.size()
                        ? call->args.size() : info.parameters.size();
                    for (size_t i = 0; i < call->args.size(); ++i) {
                        const std::string argumentType = expressionType(call->args[i]);
                        if (i < count) {
                            const Parameter& parameter = info.parameters[i];
                            if (!validArgument(parameter, call->args[i], argumentType) && error.empty()) {
                                fail("invalid argument in call to '" + call->name + "'");
                            }
                        }
                    }
                } else {
                    expressionType(expressionStatement->value);
                }
            } else {
                expressionType(expressionStatement->value);
            }
            return error.empty();
        }

        AstRef<const ReturnStmt> result = astCast<const ReturnStmt>(statement);
        if (result) {
            if (result->values.size() > currentFunction->returnTypes.size()) {
                fail("return has more values than declared function outputs");
                return false;
            }
            for (size_t i = 0; i < result->values.size(); ++i) {
                if (result->parenthesized && containsReturnCall(result->values[i])) {
                    fail("function calls are not permitted inside return parentheses");
                    return false;
                }
                const std::string type = expressionType(result->values[i]);
                if (!assignable(currentFunction->returnTypes[i], type)) {
                    fail("return type does not match function output list");
                }
            }
            return error.empty();
        }

        AstRef<const PrintStmt> print = astCast<const PrintStmt>(statement);
        if (print) {
            for (size_t i = 0; i < print->items.size(); ++i) {
                AstRef<const CallExpr> directive = astCast<const CallExpr>(print->items[i]);
                if (directive && directive->name == "prec") {
                    if (directive->args.size() != 1) {
                        fail("prec() requires exactly one integer argument");
                    } else if (expressionType(directive->args[0]) != "int") {
                        fail("prec() requires an integer argument");
                    }
                } else {
                    expressionType(print->items[i]);
                }
            }
            return error.empty();
        }

        AstRef<const IfStmt> conditional = astCast<const IfStmt>(statement);
        if (conditional) {
            if (!numeric(expressionType(conditional->condition))) fail("if condition must be numeric");
            bool valid = checkBlock(conditional->yes);
            if (conditional->no && !checkBlock(conditional->no)) valid = false;
            return valid && error.empty();
        }

        AstRef<const WhileStmt> loop = astCast<const WhileStmt>(statement);
        if (loop) {
            if (!numeric(expressionType(loop->condition))) fail("while condition must be numeric");
            ++loopDepth;
            const bool valid = checkBlock(loop->body);
            --loopDepth;
            return valid && error.empty();
        }

        AstRef<const ForStmt> range = astCast<const ForStmt>(statement);
        if (range) {
            if (borrowedFunctions.find(range->variable) != borrowedFunctions.end()) {
                fail("loop variable '" + range->variable + "' is borrowed on line " +
                     numberToString(borrowedFunctions[range->variable]) +
                     " - drop the borrow or use another name");
            }
            if (range->start && !numeric(expressionType(range->start))) fail("for start must be numeric");
            if (!numeric(expressionType(range->limit))) fail("for limit must be numeric");
            if (range->step && !numeric(expressionType(range->step))) fail("for step must be numeric");
            scopes.push_back(std::map<std::string, std::string>());
            const bool realCounter = expressionType(range->limit) == "real" ||
                (range->start && expressionType(range->start) == "real") ||
                (range->step && expressionType(range->step) == "real");
            range->realCounter = realCounter;
            scopes.back()[range->variable] = realCounter ? "real" : "int";
            ++loopDepth;
            const bool valid = checkBlock(range->body);
            --loopDepth;
            scopes.pop_back();
            return valid && error.empty();
        }

        AstRef<const SimpleStmt> simple = astCast<const SimpleStmt>(statement);
        if (simple) {
            if ((simple->keyword == "break" || simple->keyword == "continue") && loopDepth == 0) {
                fail("'" + simple->keyword + "' is only valid inside a loop");
            }
            return error.empty();
        }
        return fail("internal checker: unknown statement");
    }
};

bool SemanticChecker::check(const Program& program, std::string& error) {
    CheckerImpl checker(program);
    return checker.run(error);
}

} // namespace shalimar
