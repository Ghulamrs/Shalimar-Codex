#ifndef SHALIMAR_IR_H
#define SHALIMAR_IR_H

#include "Ast.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace shalimar {

struct IRSourceLocation {
    std::string file;
    int line;
    int column;
    IRSourceLocation() : line(0), column(0) {}
};

/* Structured operand identity. Semantic value types will be attached after
   symbol resolution; these tags never infer a Shalimar type from text. */
enum IROperandKind {
    IR_OPERAND_EMPTY,
    IR_OPERAND_TEMPORARY,
    IR_OPERAND_IDENTIFIER,
    IR_OPERAND_INTEGER_LITERAL,
    IR_OPERAND_REAL_LITERAL,
    IR_OPERAND_STRING_LITERAL,
    IR_OPERAND_COMPOSITE,
    IR_OPERAND_SPECIAL
};

struct IROperand {
    IROperandKind kind;
    std::string spelling;
    IROperand() : kind(IR_OPERAND_EMPTY) {}
    explicit IROperand(const std::string& value);
};

struct IRInstruction {
    std::string opcode;
    std::string result;
    std::string operand1;
    std::string operand2;
    IROperand resultValue;
    IROperand firstValue;
    IROperand secondValue;
    int sourceLine;
    IRSourceLocation debugLocation;

    IRInstruction(const std::string& operation, const std::string& destination,
                  const std::string& first, const std::string& second, int line)
        : opcode(operation), result(destination), operand1(first), operand2(second),
          resultValue(destination), firstValue(first), secondValue(second),
          sourceLine(line) {}
};

struct IRBasicBlock {
    int id;
    std::vector<IRInstruction> instructions;
    std::vector<int> successors;

    explicit IRBasicBlock(int blockId) : id(blockId) {}
};

struct IRLoopInduction {
    int preheaderBlock;
    int headerBlock;
    int stepBlock;
    std::string variable;
    std::string start;
    std::string step;
    std::string limit;

    IRLoopInduction(int preheader, int header, int stepBlockId,
                    const std::string& name, const std::string& initialValue,
                    const std::string& increment, const std::string& bound)
        : preheaderBlock(preheader), headerBlock(header), stepBlock(stepBlockId),
          variable(name), start(initialValue), step(increment), limit(bound) {}
};

struct IRStrengthReductionCandidate {
    std::string expression;
    std::string inductionVariable;
    std::string factor;
    std::string recurrenceStep;
    int blockId;
    bool transformed;
    std::string recurrenceTemporary;

    IRStrengthReductionCandidate(const std::string& value,
                                 const std::string& induction,
                                 const std::string& multiplier,
                                 const std::string& delta, int block,
                                 bool wasTransformed = false,
                                 const std::string& recurrence = std::string())
        : expression(value), inductionVariable(induction), factor(multiplier),
          recurrenceStep(delta), blockId(block), transformed(wasTransformed),
          recurrenceTemporary(recurrence) {}
};

struct IRType {
    std::string name;
    int arrayRank;
    bool byReference;
    IRType() : arrayRank(0), byReference(false) {}
    IRType(const std::string& n, int rank, bool reference)
        : name(n), arrayRank(rank), byReference(reference) {}
};

// Internal ABI-neutral layout for multiple scalar return values.
// Arrays are not returnable under the Shalimar 3.0 specification.
struct IRReturnField {
    IRType type;
    unsigned offset;
    unsigned size;
    IRReturnField(const IRType& t, unsigned o, unsigned s)
        : type(t), offset(o), size(s) {}
};

struct IRReturnLayout {
    std::vector<IRReturnField> fields;
    unsigned size;
    unsigned alignment;
    bool valid;
    IRReturnLayout() : size(0), alignment(1), valid(true) {}
    static unsigned alignUp(unsigned value, unsigned alignmentValue) {
        return (value + alignmentValue - 1) / alignmentValue * alignmentValue;
    }
    static IRReturnLayout fromOutputs(const std::vector<IRType>& outputs) {
        IRReturnLayout result;
        for (std::size_t i = 0; i < outputs.size(); ++i) {
            const IRType& type = outputs[i];
            unsigned width = 0;
            if (type.arrayRank != 0 || type.byReference) { result.valid = false; break; }
            if (type.name == "int") width = 4;
            else if (type.name == "real") width = 8;
            else if (type.name == "char") width = 1;
            else { result.valid = false; break; }
            const unsigned alignmentValue = width;
            result.size = alignUp(result.size, alignmentValue);
            result.fields.push_back(IRReturnField(type, result.size, width));
            result.size += width;
            if (alignmentValue > result.alignment) result.alignment = alignmentValue;
        }
        if (!result.valid) { result.fields.clear(); result.size = 0; return result; }
        result.size = alignUp(result.size, result.alignment);
        return result;
    }
};

struct IRFunctionSignature {
    std::vector<IRType> inputs;
    std::vector<IRType> outputs;
};

struct IRFunction {
    IRFunctionSignature signature;
    std::vector<std::string> parameterNames;
    std::string name;
    IRSourceLocation debugLocation;
    std::vector<IRBasicBlock> blocks;
    std::vector<IRLoopInduction> inductionVariables;
    std::vector<IRStrengthReductionCandidate> strengthReductionCandidates;
};

enum IRLibraryKind { IR_LIBRARY_EXTERNAL_MATH, IR_LIBRARY_POLYMORPHIC, IR_LIBRARY_INTRINSIC };

struct IRLibraryImport {
    IRLibraryKind kind;
    std::vector<IRType> arguments;
    std::string name;
    std::string linkerSymbol;
    int arity;
    IRType result;
    bool nativeSupported;
    IRLibraryImport() : kind(IR_LIBRARY_EXTERNAL_MATH), arity(0), nativeSupported(false) {}
};

// A resolved imported call is a semantic IR record, not a C prototype.
struct IRLibraryCall {
    std::string functionName;
    std::string resultValue;
    std::vector<IRType> argumentTypes;
    IRType resultType;
    int sourceLine;
    IRLibraryCall() : sourceLine(0) {}
};

struct IRProgram {
    std::vector<IRLibraryImport> libraryImports;
    std::vector<IRLibraryCall> libraryCalls;
    std::string sourceFile;
    std::vector<IRInstruction> globals;
    std::vector<IRFunction> functions;

    void dump(std::ostream& output) const;
};

/* Checks structural invariants and debug source locations. Not a typed semantic verifier. */
class IRVerifier {
public:
    bool verify(const IRProgram& program, std::string& error) const;
};

/* Lowers the supported AST subset to a readable three-address control-flow IR. */
class IRBuilder {
public:
    bool build(const Program& program, IRProgram& result, std::string& error) const;
};

/* Conservative Release-only IR constant propagation, folding, and dead-code pass. */
class IROptimizer {
public:
    void optimize(IRProgram& program) const;

private:
    void optimizeFunction(IRFunction& function) const;
};

/* Emits native C from optimized IR for a limited scalar and rank-one/rank-two integer-array subset. */
class IRCodeGenerator {
public:
    bool generate(const Program& source, const IRProgram& intermediate,
                  const std::string& sourcePath, std::string& output,
                  std::string& error) const;
};

} // namespace shalimar

#endif // SHALIMAR_IR_H
