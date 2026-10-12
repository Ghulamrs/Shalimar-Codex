#include "Lexer.h"
#include "Parser.h"
#include "SemanticChecker.h"
#include "IR.h"
#include "backend/tms6747/IRAssembly.h"
#include "backend/common/CheckedInteger.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static int failures = 0;
static void expect(bool condition, const char* reason) {
    if (!condition) { std::cerr << reason << '\n'; ++failures; }
}
static bool lower(const std::string& source, shalimar::IRProgram& ir, std::string& error) {
    std::vector<shalimar::Token> tokens;
    shalimar::Program program;
    if (!shalimar::Lexer().tokenize(source, tokens, error) ||
        !shalimar::Parser(program).parse(tokens, program, error) ||
        !shalimar::SemanticChecker().check(program, error)) return false;
    ir.sourceFile = "repair-test.shl";
    return shalimar::IRBuilder().build(program, ir, error) && shalimar::IRVerifier().verify(ir, error);
}
int main() {
    int result = 0;
    expect(!shalimar::checkedIntegerBinary("+", INT_MAX, 1, result), "constant addition overflow accepted");
    expect(!shalimar::checkedIntegerBinary("-", INT_MIN, 1, result), "constant subtraction overflow accepted");
    expect(!shalimar::checkedIntegerBinary("*", INT_MIN, -1, result), "constant multiplication overflow accepted");
    expect(shalimar::checkedIntegerBinary("+", INT_MIN, INT_MAX, result) && result == -1, "valid signed boundary rejected");
    expect(shalimar::signedIntegerBits(static_cast<unsigned>(-1)) == -1, "negative bit pattern lost");
    std::string error, assembly;
    shalimar::IRProgram ir;
    expect(lower("fun main() { real X[][] : {{1.,,},{,2.,},{,,5.}} }", ir, error), "rectangular inferred omitted slots rejected");
    std::ostringstream arrayDump;
    ir.dump(arrayDump);
    expect(arrayDump.str().find("declare real 3, 3") != std::string::npos, "array extents incorrectly inferred");
    expect(!lower("fun main() { int A[5] : {10,20} }", ir, error), "partial initializer accepted");
    expect(!lower("fun main() { int A[2+3] }", ir, error), "expression array extent accepted");
    expect(!lower("fun main() { char S[5] : \"hover\" }", ir, error), "string capacity overflow accepted");
    expect(lower("fun main() { char S[] : \"hi\" }", ir, error), "inferred char capacity rejected");
    std::ostringstream charDump;
    ir.dump(charDump);
    expect(charDump.str().find("declare char 3") != std::string::npos, "terminator omitted from char capacity");
    expect(lower("fun <int> = f(i: int) { int A[3] A[i] : i+2147483647 return A[i] } fun main() {}", ir, error), "index ordering probe failed");
    std::ostringstream indexDump;
    ir.dump(indexDump);
    expect(indexDump.str().find("index.check") < indexDump.str().find("binary +"), "bounds check follows assignment value");
    expect(!lower("uses sqrt\nfun <real> = f(sqrt: real) { return sqrt }\nfun main() {}", ir, error),
           "borrowed parameter collision accepted");
    expect(error.find("parameter 'sqrt'") != std::string::npos && error.find("line 2") != std::string::npos,
           "borrowed parameter location missing");
    expect(!lower("fun <int> = f(x: int, x: int) { return x }\nfun main() {}", ir, error),
           "duplicate parameter accepted");
    expect(error.find("duplicate parameter") != std::string::npos, "duplicate parameter diagnostic missing");
    expect(!lower("fun main() {\nint x : missing\n}", ir, error), "undefined name accepted");
    expect(error.find("line 2") != std::string::npos, "statement source line missing");
    expect(lower("fun <int> = f(x: int) { if x { return 1 } else { return 2 } }\nfun main() {}", ir, error),
           "valid return branches rejected");
    for (std::size_t f = 0; f < ir.functions.size(); ++f)
        for (std::size_t b = 0; b < ir.functions[f].blocks.size(); ++b) {
            const shalimar::IRBasicBlock& block = ir.functions[f].blocks[b];
            for (std::size_t i = 0; i < block.instructions.size(); ++i)
                if (block.instructions[i].opcode == "return")
                    expect(i + 1 == block.instructions.size() && block.successors.empty(), "return has trailing code or successors");
        }
    expect(lower("fun <int> = f() { int x return x }\nfun main() {}", ir, error), "frontend uninitialized fixture failed");
    expect(!shalimar::tms6747::IRAssembly().emit(ir, assembly, error, false), "DSP uninitialized load accepted");
    expect(error.find("before initialization") != std::string::npos, "DSP initialization diagnostic missing");
    expect(lower("fun <int> = f(c: int) { int x if c { x : 1 } return x }\nfun main() {}", ir, error),
           "frontend conditional initialization fixture failed");
    expect(!shalimar::tms6747::IRAssembly().emit(ir, assembly, error, false), "DSP join lost uninitialized path");
    expect(lower("fun <int> = f(c: int) { int x if c { x : 1 } else { x : 2 } return x }\nfun main() {}", ir, error),
           "fully initialized branch fixture failed");
    expect(shalimar::tms6747::IRAssembly().emit(ir, assembly, error, false), "DSP rejects all-path initialization");
    expect(lower("fun <int> = f(c: int) { int x while c { x : 1 c : 0 } return x }\nfun main() {}", ir, error),
           "loop initialization fixture failed");
    expect(!shalimar::tms6747::IRAssembly().emit(ir, assembly, error, false), "DSP assumes loop executes");
    expect(lower("fun <int> = f() { return 1 }\nfun main() {}", ir, error), "valid constant return failed");
    ir.functions[0].blocks[0].instructions.push_back(shalimar::IRInstruction("constant", "%t999", "2", "", 1));
    expect(!shalimar::IRVerifier().verify(ir, error), "verifier accepts code after terminator");
    expect(lower("fun <int> = f() { return 2147483647 + 1 }\nfun main() {}", ir, error), "constant overflow lowering failed");
    expect(!shalimar::tms6747::IRAssembly().emit(ir, assembly, error, false), "DSP constant overflow accepted");
    expect(lower("fun <real> = f() { real x : 3 return x / 2 }\nfun main() {}", ir, error), "real arithmetic fixture failed");
    shalimar::IROptimizer().optimize(ir);
    std::ostringstream dump;
    ir.dump(dump);
    expect(dump.str().find("binary / x, 2") != std::string::npos, "optimizer substituted integer facts for real local");
    expect(lower("fun <real> = f() { real x : 2147483647 return x + 1 }\nfun main() {}", ir, error), "real addition fixture failed");
    shalimar::IROptimizer().optimize(ir);
    expect(shalimar::tms6747::IRAssembly().emit(ir, assembly, error, false), "real addition mistaken for integer overflow");
    if (failures) return 1;
    std::cout << "Rev3 semantic, CFG and DSP initialization contracts passed\n";
    return 0;
}
