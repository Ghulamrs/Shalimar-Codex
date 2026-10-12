#include "IR.h"

#include <cstdio>
#include <map>
#include <ostream>
#include <sstream>

namespace shalimar {
namespace {

std::string numberText(int value) {
    std::ostringstream output;
    output << value;
    return output.str();
}

std::string quoteIRString(const std::string& text) {
    std::string result = "\"";
    for (size_t i = 0; i < text.size(); ++i) {
        const unsigned char character = static_cast<unsigned char>(text[i]);
        if (character == '\\' || character == '\"') {
            result += '\\';
            result += static_cast<char>(character);
        } else if (character == '\n') {
            result += "\\n";
        } else if (character == '\r') {
            result += "\\r";
        } else if (character == '\t') {
            result += "\\t";
        } else if (character < 32) {
            char escaped[8];
            std::sprintf(escaped, "\\%03o", character);
            result += escaped;
        } else {
            result += static_cast<char>(character);
        }
    }
    result += '"';
    return result;
}

std::string literalText(const LiteralExpr& literal) {
    if (literal.stringValue) return quoteIRString(literal.value);
    return literal.value;
}

class FunctionLowerer {
public:
    FunctionLowerer(IRFunction& function, const Program& program)
        : function_(function), program_(program), nextTemp_(0) {
        newBlock();
    }

    void lower(AstRef<const BlockStmt> block) {
        lowerBlock(block);
    }

    void lowerGlobal(AstRef<const DeclStmt> declaration) {
        if (!declaration) return;
        std::string dimensions;
        for (size_t i = 0; i < declaration->dimensions.size(); ++i) {
            if (i) dimensions += ", ";
            dimensions += lowerExpression(declaration->dimensions[i], declaration->sourceLine);
        }
        emit("global " + declaration->type, declaration->name, dimensions, "",
             declaration->sourceLine);
        if (declaration->init) {
            const std::string value = lowerExpression(declaration->init, declaration->sourceLine);
            emit("global.initialize", declaration->name, value, "", declaration->sourceLine);
        }
    }

private:
    IRFunction& function_;
    const Program& program_;
    int nextTemp_;
    int currentBlock_;

    int createBlock() {
        const int id = static_cast<int>(function_.blocks.size());
        function_.blocks.push_back(IRBasicBlock(id));
        return id;
    }

    int newBlock() {
        const int id = createBlock();
        currentBlock_ = id;
        return id;
    }

    void switchTo(int id) {
        currentBlock_ = id;
    }

    IRBasicBlock& current() {
        return function_.blocks[static_cast<size_t>(currentBlock_)];
    }

    bool terminated() {
        if (current().instructions.empty()) return false;
        const std::string& op = current().instructions.back().opcode;
        return op == "return" || op == "jump" || op == "branch";
    }

    void emit(const std::string& opcode, const std::string& result,
              const std::string& first, const std::string& second, int line) {
        if (terminated()) return;
        current().instructions.push_back(IRInstruction(opcode, result, first, second, line));
    }

    void connect(int target) {
        if (!current().instructions.empty() && current().instructions.back().opcode == "return") return;
        std::vector<int>& successors = current().successors;
        for (size_t i = 0; i < successors.size(); ++i) {
            if (successors[i] == target) return;
        }
        successors.push_back(target);
    }

    std::string temporary() {
        return "%t" + numberText(nextTemp_++);
    }

    bool collectIndexParts(AstRef<const Expr> expression, AstRef<const NameExpr>& base,
                           std::vector<AstRef<const Expr> >& indices) const {
        AstRef<const IndexExpr> indexed = astCast<const IndexExpr>(expression);
        if (!indexed) {
            base = astCast<const NameExpr>(expression);
            return base != 0;
        }
        if (!collectIndexParts(indexed->base, base, indices)) return false;
        indices.push_back(indexed->index);
        return true;
    }

    std::string lowerExpression(AstRef<const Expr> expression, int line) {
        if (!expression) return "<none>";
        const LiteralExpr* literal = dynamic_cast<const LiteralExpr*>(&expression.value());
        if (literal) return literalText(*literal);
        const NameExpr* name = dynamic_cast<const NameExpr*>(&expression.value());
        if (name) return name->constantValue.empty() ? name->name : name->constantValue;

        const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(&expression.value());
        if (unary) {
            const std::string operand = lowerExpression(unary->value, line);
            const std::string result = temporary();
            emit("unary " + unary->op, result, operand, "", line);
            return result;
        }

        const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(&expression.value());
        if (binary) {
            const std::string left = lowerExpression(binary->left, line);
            const std::string right = lowerExpression(binary->right, line);
            const std::string result = temporary();
            emit("binary " + binary->op, result, left, right, line);
            return result;
        }

        const IndexExpr* index = dynamic_cast<const IndexExpr*>(&expression.value());
        if (index) {
            AstRef<const NameExpr> baseName = 0;
            std::vector<AstRef<const Expr> > indexExpressions;
            if (collectIndexParts(expression, baseName, indexExpressions) && baseName) {
                std::string indices;
                for (size_t i = 0; i < indexExpressions.size(); ++i) {
                    if (i) indices += ", ";
                    indices += lowerExpression(indexExpressions[i], line);
                }
                const std::string result = temporary();
                emit("index", result, baseName->name, indices, line);
                return result;
            }
            const std::string base = lowerExpression(index->base, line);
            const std::string subscript = lowerExpression(index->index, line);
            const std::string result = temporary();
            emit("index", result, base, subscript, line);
            return result;
        }

        const AttributeExpr* attribute = dynamic_cast<const AttributeExpr*>(&expression.value());
        if (attribute) {
            const std::string base = lowerExpression(attribute->base, line);
            const std::string axis = lowerExpression(attribute->axis, line);
            const std::string result = temporary();
            emit("attribute " + attribute->attribute, result, base, axis, line);
            return result;
        }

        const ArrayLiteralExpr* array = dynamic_cast<const ArrayLiteralExpr*>(&expression.value());
        if (array) {
            const std::string result = temporary();
            emit("array.begin", result, numberText(static_cast<int>(array->values.size())), "", line);
            for (size_t i = 0; i < array->values.size(); ++i) {
                const std::string value = array->values[i]
                    ? lowerExpression(array->values[i], line) : "0";
                emit("array.element", result, numberText(static_cast<int>(i)), value, line);
            }
            return result;
        }

        const CallExpr* call = dynamic_cast<const CallExpr*>(&expression.value());
        if (call) {
            if (call->name == "len" && call->args.size() == 1) {
                AstRef<const NameExpr> arrayName = astCast<const NameExpr>(call->args[0]);
                if (arrayName) {
                    const std::string result = temporary();
                    emit("array.len", result, arrayName->name, "", line);
                    return result;
                }
            }
            std::string arguments;
            FunctionId target = program_.findNamedFunction(call->name);
            const bool hasTarget = target.valid();
            const size_t parameterCount = hasTarget ? program_.arena.function(target).parameters.size() : 0;
            for (size_t i = 0; i < call->args.size(); ++i) {
                if (i) arguments += ", ";
                const bool hasParameter = hasTarget && i < parameterCount;
                const bool byReferenceScalar = hasParameter &&
                    program_.arena.function(target).parameters[i].byReference &&
                    program_.arena.function(target).parameters[i].arrayRank == 0;
                AstRef<const NameExpr> base = 0;
                std::vector<AstRef<const Expr> > indices;
                if (byReferenceScalar &&
                    collectIndexParts(call->args[i], base, indices) && base &&
                    !indices.empty()) {
                    arguments += "@refindex|" + base->name;
                    for (size_t d = 0; d < indices.size(); ++d) {
                        arguments += "|" + lowerExpression(indices[d], line);
                    }
                } else {
                    arguments += lowerExpression(call->args[i], line);
                }
            }
            const std::string result = temporary();
            emit("call " + call->name, result, arguments, "", line);
            return result;
        }
        return "<unsupported-expression>";
    }

    void lowerBlock(AstRef<const BlockStmt> block) {
        if (!block) return;
        for (size_t i = 0; i < block->statements.size(); ++i) {
            if (terminated()) break;
            lowerStatement(block->statements[i]);
        }
    }

    void lowerStatement(AstRef<const Stmt> statement) {
        if (!statement) return;
        const int line = statement->sourceLine;

        AstRef<const BlockStmt> nested = astCast<const BlockStmt>(statement);
        if (nested) { lowerBlock(nested); return; }

        AstRef<const DeclStmt> declaration = astCast<const DeclStmt>(statement);
        if (declaration) {
            std::string dimensions;
            for (size_t i = 0; i < declaration->dimensions.size(); ++i) {
                if (i) dimensions += ", ";
                dimensions += lowerExpression(declaration->dimensions[i], line);
            }
            emit("declare " + declaration->type, declaration->name, dimensions, "", line);
            if (declaration->init) {
                const std::string value = lowerExpression(declaration->init, line);
                emit("initialize", declaration->name, value, "", line);
            }
            return;
        }

        AstRef<const AssignStmt> assignment = astCast<const AssignStmt>(statement);
        if (assignment) {
            AstRef<const IndexExpr> indexedTarget = astCast<const IndexExpr>(assignment->target);
            if (indexedTarget) {
                AstRef<const NameExpr> arrayName = 0;
                std::vector<AstRef<const Expr> > indexExpressions;
                if (collectIndexParts(assignment->target, arrayName, indexExpressions) && arrayName) {
                    std::string indices;
                    for (size_t i = 0; i < indexExpressions.size(); ++i) {
                        if (i) indices += ", ";
                        indices += lowerExpression(indexExpressions[i], line);
                    }
                    const std::string value = lowerExpression(assignment->value, line);
                    emit("index.store " + assignment->op, arrayName->name, indices, value, line);
                    return;
                }
            }
            const std::string target = lowerExpression(assignment->target, line);
            const std::string value = lowerExpression(assignment->value, line);
            emit("assign " + assignment->op, target, value, "", line);
            return;
        }

        AstRef<const MultiAssignStmt> multi = astCast<const MultiAssignStmt>(statement);
        if (multi) {
            std::string names;
            for (size_t i = 0; i < multi->names.size(); ++i) {
                if (i) names += ", ";
                names += multi->names[i];
            }
            std::string callValue = "<none>";
            if (multi->call) callValue = lowerExpression(multi->call, line);
            emit("multi.assign", names, callValue, "", line);
            return;
        }

        AstRef<const ExprStmt> expression = astCast<const ExprStmt>(statement);
        if (expression) {
            emit("evaluate", "", lowerExpression(expression->value, line), "", line);
            return;
        }

        AstRef<const ReturnStmt> returned = astCast<const ReturnStmt>(statement);
        if (returned) {
            std::string values;
            for (size_t i = 0; i < returned->values.size(); ++i) {
                if (i) values += ", ";
                values += lowerExpression(returned->values[i], line);
            }
            // Every omitted trailing output is explicitly initialized to its typed zero.
            for (size_t i = returned->values.size(); i < function_.signature.outputs.size(); ++i) {
                if (!values.empty()) values += ", ";
                const std::string& type = function_.signature.outputs[i].name;
                if (type == "real") values += "0.0";
                else if (type == "char") values += "char(0)";
                else values += "0";
            }
            emit("return", "", values, "", line);
            return;
        }

        AstRef<const PrintStmt> printed = astCast<const PrintStmt>(statement);
        if (printed) {
            for (size_t i = 0; i < printed->items.size(); ++i) {
                AstRef<const CallExpr> directive = astCast<const CallExpr>(printed->items[i]);
                if (directive && directive->name == "prec") {
                    const std::string precision = directive->args.empty()
                        ? "<unsupported-expression>"
                        : lowerExpression(directive->args[0], line);
                    emit("print.precision", "", precision, "", line);
                    continue;
                }
                emit("print", "", lowerExpression(printed->items[i], line), "", line);
                emit("print.separator", "", "", "", line);
            }
            if (printed->newline) emit("print.newline", "", "", "", line);
            return;
        }

        AstRef<const IfStmt> conditional = astCast<const IfStmt>(statement);
        if (conditional) {
            const std::string condition = lowerExpression(conditional->condition, line);
            const int yesBlock = createBlock();
            const int noBlock = createBlock();
            const int mergeBlock = createBlock();
            emit("branch", "", condition,
                 "bb" + numberText(yesBlock) + ", bb" + numberText(noBlock), line);
            connect(yesBlock);
            connect(noBlock);
            switchTo(yesBlock);
            lowerBlock(conditional->yes);
            emit("jump", "", "bb" + numberText(mergeBlock), "", line);
            connect(mergeBlock);
            switchTo(noBlock);
            lowerBlock(conditional->no);
            emit("jump", "", "bb" + numberText(mergeBlock), "", line);
            connect(mergeBlock);
            switchTo(mergeBlock);
            return;
        }

        AstRef<const WhileStmt> whileLoop = astCast<const WhileStmt>(statement);
        if (whileLoop) {
            const int conditionBlock = createBlock();
            const int bodyBlock = createBlock();
            const int exitBlock = createBlock();
            emit("jump", "", "bb" + numberText(conditionBlock), "", line);
            connect(conditionBlock);
            switchTo(conditionBlock);
            const std::string condition = lowerExpression(whileLoop->condition, line);
            emit("branch", "", condition,
                 "bb" + numberText(bodyBlock) + ", bb" + numberText(exitBlock), line);
            connect(bodyBlock);
            connect(exitBlock);
            switchTo(bodyBlock);
            lowerBlock(whileLoop->body);
            emit("jump", "", "bb" + numberText(conditionBlock), "", line);
            connect(conditionBlock);
            switchTo(exitBlock);
            return;
        }

        AstRef<const ForStmt> forLoop = astCast<const ForStmt>(statement);
        if (forLoop) {
            const std::string start = forLoop->start ? lowerExpression(forLoop->start, line) : "0";
            emit("assign =", forLoop->variable, start, "", line);
            const int conditionBlock = createBlock();
            const int bodyBlock = createBlock();
            const int stepBlock = createBlock();
            const int exitBlock = createBlock();
            emit("jump", "", "bb" + numberText(conditionBlock), "", line);
            connect(conditionBlock);
            switchTo(conditionBlock);
            std::string limit = lowerExpression(forLoop->limit, line);
            if (!forLoop->start) {
                const std::string adjusted = temporary();
                emit("binary -", adjusted, limit, "1", line);
                limit = adjusted;
            }
            const std::string condition = temporary();
            emit("for.test", condition, forLoop->variable, limit, line);
            emit("branch", "", condition,
                 "bb" + numberText(bodyBlock) + ", bb" + numberText(exitBlock), line);
            connect(bodyBlock);
            connect(exitBlock);
            switchTo(bodyBlock);
            lowerBlock(forLoop->body);
            emit("jump", "", "bb" + numberText(stepBlock), "", line);
            connect(stepBlock);
            switchTo(stepBlock);
            const std::string step = forLoop->step ? lowerExpression(forLoop->step, line) : "1";
            emit("for.step", forLoop->variable, step, "", line);
            emit("jump", "", "bb" + numberText(conditionBlock), "", line);
            connect(conditionBlock);
            switchTo(exitBlock);
            return;
        }

        AstRef<const SimpleStmt> simple = astCast<const SimpleStmt>(statement);
        if (simple) emit(simple->keyword, "", "", "", line);
    }
};

void dumpInstruction(std::ostream& output, const IRInstruction& instruction) {
    output << "    ";
    if (!instruction.result.empty()) output << instruction.result << " = ";
    output << instruction.opcode;
    if (!instruction.operand1.empty()) output << ' ' << instruction.operand1;
    if (!instruction.operand2.empty()) output << ", " << instruction.operand2;
    if (instruction.sourceLine > 0) output << "  ; line " << instruction.sourceLine;
    output << '\n';
}

} // namespace

bool IRBuilder::build(const Program& program, IRProgram& result, std::string& error) const {
    result.globals.clear();
    result.functions.clear();
    result.libraryImports.clear();
    result.libraryCalls.clear();
    // Imports are explicit IR dependencies, never implicit C runtime calls.
    for (size_t u = 0; u < program.libraryUses.size(); ++u) {
        IRLibraryImport imported;
        imported.name = program.libraryUses[u].name;
        imported.linkerSymbol = imported.name;
        imported.arity = (imported.name == "pow" || imported.name == "hypot" ||
                          imported.name == "atan2" || imported.name == "fmod" ||
                          imported.name == "min" || imported.name == "max") ? 2 : 1;
        // Library declarations are not C prototypes: the IR records the
        // semantic family and leaves polymorphic resolution to typed calls.
        imported.kind = imported.name == "len" ? IR_LIBRARY_INTRINSIC :
            (imported.name == "abs" || imported.name == "min" || imported.name == "max") ?
            IR_LIBRARY_POLYMORPHIC : IR_LIBRARY_EXTERNAL_MATH;
        imported.result = IRType(imported.kind == IR_LIBRARY_INTRINSIC ? "int" :
                                 imported.kind == IR_LIBRARY_POLYMORPHIC ? "unresolved" : "real", 0, false);
        for (int a = 0; a < imported.arity; ++a)
            imported.arguments.push_back(IRType(imported.kind == IR_LIBRARY_INTRINSIC ? "array-or-string" :
                                               imported.kind == IR_LIBRARY_POLYMORPHIC ? "numeric" : "real", 0, false));
        imported.nativeSupported = false; // No native ABI lowering yet.
        result.libraryImports.push_back(imported);
    }
    // rev42: independently resolve imported math calls whose operands are
    // literal numeric values. Unknown types remain explicitly unresolved.
    // This is semantic metadata only; no C ABI is inferred from it.
    error.clear();

    IRFunction globalFunction;
    globalFunction.name = "__globals";
    FunctionLowerer globalLowerer(globalFunction, program);
    for (size_t i = 0; i < program.globals.size(); ++i) {
        globalLowerer.lowerGlobal(program.globals[i]);
    }
    if (!globalFunction.blocks.empty()) {
        result.globals = globalFunction.blocks[0].instructions;
    }

    for (size_t i = 0; i < program.functions.size(); ++i) {
        AstRef<const FunctionDef> sourceFunction = program.functions[i];
        IRFunction function;
        function.name = sourceFunction->name;
        function.debugLocation.file = result.sourceFile;
        function.debugLocation.line = sourceFunction->sourceLine;
        for (size_t p = 0; p < sourceFunction->parameters.size(); ++p) {
            const Parameter& parameter = sourceFunction->parameters[p];
            function.signature.inputs.push_back(IRType(parameter.type, parameter.arrayRank,
                                                       parameter.byReference));
            function.parameterNames.push_back(parameter.name);
        }
        for (size_t r = 0; r < sourceFunction->returnTypes.size(); ++r) {
            function.signature.outputs.push_back(IRType(sourceFunction->returnTypes[r], 0, false));
        }
        FunctionLowerer lowerer(function, program);
        lowerer.lower(sourceFunction->body);
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            for (size_t n = 0; n < function.blocks[b].instructions.size(); ++n) {
                IRInstruction& instruction = function.blocks[b].instructions[n];
                instruction.debugLocation.file = result.sourceFile;
                instruction.debugLocation.line = instruction.sourceLine;
            }
        }
        result.functions.push_back(function);
    }
    // Resolve imported call-site signatures from the lowered operands.
    // Nonliteral expressions remain explicitly unresolved until typed SSA
    // propagation is available. Never guess an ABI from a textual operand.
    for (size_t fi = 0; fi < result.functions.size(); ++fi) {
        const IRFunction& function = result.functions[fi];
        std::map<std::string, IRType> knownTypes;
        for (size_t pi = 0; pi < function.parameterNames.size(); ++pi)
            knownTypes[function.parameterNames[pi]] = function.signature.inputs[pi];
        for (size_t bi = 0; bi < function.blocks.size(); ++bi) {
            const IRBasicBlock& block = function.blocks[bi];
            for (size_t ii = 0; ii < block.instructions.size(); ++ii) {
                const IRInstruction& inst = block.instructions[ii];
                if (inst.opcode.compare(0, 5, "call ") != 0) {
                    // Track only unambiguous scalar declarations and copies.
                    if (inst.opcode == "declare int" || inst.opcode == "declare real")
                        knownTypes[inst.result] = IRType(inst.opcode.substr(8), 0, false);
                    continue;
                }
                const std::string name = inst.opcode.substr(5);
                for (size_t ui = 0; ui < result.libraryImports.size(); ++ui) {
                    const IRLibraryImport& imported = result.libraryImports[ui];
                    if (imported.name != name) continue;
                    IRLibraryCall call;
                    call.functionName = name;
                    call.resultValue = inst.result;
                    call.sourceLine = inst.sourceLine;
                    // Token splitting is safe only for scalar literals. All
                    // composite operands stay unresolved, not misclassified.
                    std::istringstream parts(inst.operand1);
                    std::string part;
                    while (std::getline(parts, part, ',')) {
                        const std::string::size_type first = part.find_first_not_of(" \t");
                        const std::string::size_type last = part.find_last_not_of(" \t");
                        if (first == std::string::npos) continue;
                        part = part.substr(first, last - first + 1);
                        IRType type("unresolved", 0, false);
                        if (!part.empty() && (part[0] == '-' || part[0] == '+' ||
                            (part[0] >= '0' && part[0] <= '9'))) {
                            const std::string::size_type start = (part[0] == '-' || part[0] == '+') ? 1 : 0;
                            bool numeric = start < part.size();
                            bool real = false;
                            for (size_t k = start; k < part.size(); ++k) {
                                if (part[k] == '.') real = true;
                                else if (part[k] < '0' || part[k] > '9') numeric = false;
                            }
                            if (numeric) type.name = real ? "real" : "int";
                        }
                        if (type.name == "unresolved") {
                            std::map<std::string, IRType>::const_iterator found = knownTypes.find(part);
                            if (found != knownTypes.end()) type = found->second;
                        }
                        call.argumentTypes.push_back(type);
                    }
                    call.resultType = imported.result;
                    if (imported.kind == IR_LIBRARY_EXTERNAL_MATH) {
                        call.resultType = IRType("real", 0, false);
                    } else if (imported.kind == IR_LIBRARY_POLYMORPHIC &&
                               !call.argumentTypes.empty()) {
                        const std::string firstType = call.argumentTypes[0].name;
                        bool same = firstType == "int" || firstType == "real";
                        for (size_t ai = 1; ai < call.argumentTypes.size(); ++ai)
                            if (call.argumentTypes[ai].name != firstType) same = false;
                        if (same) call.resultType = IRType(firstType, 0, false);
                    }
                    if (call.resultType.name == "int" || call.resultType.name == "real")
                        knownTypes[inst.result] = call.resultType;
                    result.libraryCalls.push_back(call);
                    break;
                }
            }
        }
    }
    return true;
}

void IRProgram::dump(std::ostream& output) const {
    output << "shalimar-ir v1\n";
    if (!libraryCalls.empty()) {
        output << "library-calls:\n";
        for (size_t ci = 0; ci < libraryCalls.size(); ++ci) {
            const IRLibraryCall& call = libraryCalls[ci];
            output << "  " << call.functionName << "(";
            for (size_t ai = 0; ai < call.argumentTypes.size(); ++ai) {
                if (ai) output << ", ";
                output << call.argumentTypes[ai].name;
            }
            output << ") -> " << call.resultType.name << "\n";
        }
    }
    if (!globals.empty()) {
        output << "globals:\n";
        for (size_t i = 0; i < globals.size(); ++i) dumpInstruction(output, globals[i]);
    }
    for (size_t i = 0; i < functions.size(); ++i) {
        const IRFunction& function = functions[i];
        output << "\nfunction @" << function.name << " (";
        for (size_t p = 0; p < function.signature.inputs.size(); ++p) {
            if (p) output << ", ";
            const IRType& type = function.signature.inputs[p];
            if (type.byReference) output << "ref ";
            output << type.name;
            for (int rank = 0; rank < type.arrayRank; ++rank) output << "[]";
        }
        output << ") -> (";
        for (size_t r = 0; r < function.signature.outputs.size(); ++r) {
            if (r) output << ", ";
            output << function.signature.outputs[r].name;
        }
        output << "):\n";
        for (size_t loop = 0; loop < function.inductionVariables.size(); ++loop) {
            const IRLoopInduction& induction = function.inductionVariables[loop];
            output << "  loop-induction " << induction.variable
                   << " start=" << induction.start
                   << " step=" << induction.step
                   << " limit=" << induction.limit
                   << " preheader=bb" << induction.preheaderBlock
                   << " header=bb" << induction.headerBlock
                   << " step-block=bb" << induction.stepBlock << '\n';
        }
        for (size_t candidate = 0; candidate < function.strengthReductionCandidates.size(); ++candidate) {
            const IRStrengthReductionCandidate& item = function.strengthReductionCandidates[candidate];
            output << "  strength-reduction "
                   << (item.transformed ? "applied" : "opportunity")
                   << " expression=" << item.expression
                   << " induction=" << item.inductionVariable
                   << " factor=" << item.factor
                   << " recurrence-step=" << item.recurrenceStep
                   << " block=bb" << item.blockId;
            if (item.transformed) output << " recurrence=" << item.recurrenceTemporary;
            else output << " (not transformed; safety proof unavailable)";
            output << '\n';
        }
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            const IRBasicBlock& block = function.blocks[b];
            output << "  bb" << block.id << ":\n";
            for (size_t n = 0; n < block.instructions.size(); ++n) {
                dumpInstruction(output, block.instructions[n]);
            }
            if (!block.successors.empty()) {
                output << "    successors:";
                for (size_t s = 0; s < block.successors.size(); ++s) {
                    output << " bb" << block.successors[s];
                }
                output << '\n';
            }
        }
    }
}

} // namespace shalimar
