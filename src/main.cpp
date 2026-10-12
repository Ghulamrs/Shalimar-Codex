#include "backend/tms6747/IRAssembly.h"
#include "backend/tms6747/Toolchain.h"
#include <cstdlib>
#include "backend/coff/CoffWriter.h"
#include <fstream>
#include "backend/x86_64/IRSelector.h"
#include "backend/x86_64/InstructionEncoder.h"
#include "Lexer.h"
#include "IR.h"
#include "Parser.h"
#include "SemanticChecker.h"

#include <fstream>
#include <iomanip>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

class SourceReader {
public:
    bool read(const std::string& path, std::string& text) const {
        std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (!file) return false;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        text = buffer.str();
        return file.good() || file.eof();
    }
};

std::size_t instructionCount(const shalimar::IRProgram& ir) {
    std::size_t count = ir.globals.size();
    for (std::size_t f = 0; f < ir.functions.size(); ++f)
        for (std::size_t b = 0; b < ir.functions[f].blocks.size(); ++b)
            count += ir.functions[f].blocks[b].instructions.size();
    return count;
}

class CompilerDriver {
public:
    int run(int argumentCount, char** arguments) const {
        if (argumentCount < 3) {
            printUsage();
            return 2;
        }

        const std::string inputPath = arguments[1];
        std::string mode;
        std::string outputPath;
        bool releaseMode = false;
        bool debugSymbols = false;
        bool optimizationReport = false;
        bool diagnosticStages = false;
        std::string debugMapPath;
        std::string variableReportPath;
        std::string liveRangeReportPath;
        bool dspTarget = false;
        std::string selectedTarget = "windows-x86_64";
        std::string dspDebugIndexPath;
        const char* configuredTiRoot = std::getenv("TI_C6000_ROOT");
        std::string tiRoot = configuredTiRoot ? configuredTiRoot : "C:/ti/ccsv5/tools/compiler/c6000_7.4.4";
        std::string dspEntry = "main";
        bool ownDspTools = true;
        const char* configuredTools = std::getenv("SHALIMAR_C6747_TOOLS");
        std::string toolsRoot = configuredTools ? configuredTools : "C:/Users/GRA/source";
        std::string runtimePath;

        for (int i = 2; i < argumentCount; ++i) {
            const std::string argument = arguments[i];
            if (argument == "--check" || argument == "--dump-ir" || argument == "--object" ||
                argument == "-S" || argument == "--executable") {
                if (!mode.empty()) { std::cerr << "shalimarc: conflicting output modes\n"; return 2; }
                mode = argument;
            } else if (argument == "-o" && i + 1 < argumentCount) {
                outputPath = arguments[++i];
            } else if (argument == "--ti-root" && i + 1 < argumentCount) {
                tiRoot = arguments[++i];
            } else if (argument == "--tools-root" && i + 1 < argumentCount) {
                toolsRoot = arguments[++i];
            } else if (argument == "--rts-lib" && i + 1 < argumentCount) {
                runtimePath = arguments[++i];
            } else if (argument == "--toolchain=shalimar") {
                ownDspTools = true;
            } else if (argument == "--toolchain=ti") {
                ownDspTools = false;
            } else if (argument == "--dsp-entry" && i + 1 < argumentCount) {
                dspEntry = arguments[++i];
            } else if (argument == "--target=tms6747") {
                dspTarget = true;
                selectedTarget = "tms6747";
            } else if (argument == "--target=windows-x86_64" ||
                       argument == "--target=linux-x86_64" ||
                       argument == "--target=macos-arm64") {
                selectedTarget = argument.substr(9);
                dspTarget = false;
            } else if (argument == "--dsp-debug-index" && i + 1 < argumentCount) {
                dspDebugIndexPath = arguments[++i];
            } else if (argument == "--release") {
                releaseMode = true;
            } else if (argument == "--middle=s") {
                // Accepted for compatibility; S is the only compilation pipeline.
            } else if (argument == "--middle=c" || argument == "--ir-native" ||
                       argument == "--emit-c" || argument == "--cc") {
                std::cerr << "shalimarc: C backends and C compiler options have been removed; use native S mode\n";
                return 2;
            } else if (argument == "--optimization-report") {
                optimizationReport = true;
            } else if (argument == "--diagnostic-stages") {
                diagnosticStages = true;
            } else if (argument == "--debug-map" && i + 1 < argumentCount) {
                debugMapPath = arguments[++i];
            } else if (argument == "--debug-live-ranges" && i + 1 < argumentCount) {
                liveRangeReportPath = arguments[++i];
            } else if (argument == "--debug-variables" && i + 1 < argumentCount) {
                variableReportPath = arguments[++i];
            } else if (argument == "--debug-symbols") {
                debugSymbols = true;
            } else if (argument == "--debug") {
                releaseMode = false;
            } else if (outputPath.empty() && (mode == "--object" || mode == "-S") && argument.find("-") != 0) {
                outputPath = argument;
            } else {
                std::cerr << "Unknown argument: " << argument << '\n';
                return 2;
            }
        }

        if (mode.empty() && !outputPath.empty()) mode = "--executable";
        if ((selectedTarget == "linux-x86_64" || selectedTarget == "macos-arm64") &&
            mode != "--check" && mode != "--dump-ir") {
            std::cerr << "shalimarc: native output for " << selectedTarget
                      << " is not implemented; only frontend checking and IR dumps are available\n";
            return 2;
        }
        if (dspTarget && (!debugMapPath.empty() || !variableReportPath.empty() ||
                          !liveRangeReportPath.empty())) {
            std::cerr << "shalimarc: --debug-map, --debug-variables and --debug-live-ranges "
                         "are Windows x64 reports; use --dsp-debug-index with -S for TMS6747\n";
            return 2;
        }
        if (!dspDebugIndexPath.empty() && mode != "-S") {
            std::cerr << "shalimarc: --dsp-debug-index requires -S\n";
            return 2;
        }
        if (debugSymbols && mode != "--object" && mode != "-S" && mode != "--executable") {
            std::cerr << "shalimarc: --debug-symbols requires native output\n";
            return 2;
        }
        if (!dspTarget && mode == "--executable") {
            std::cerr << "shalimarc: integrated executable output currently requires --target=tms6747\n"; return 2;
        }
        if (!dspTarget && mode == "-S") {
            std::cerr << "shalimarc: -S requires --target=tms6747\n";
            return 2;
        }
        if (!dspDebugIndexPath.empty() && !dspTarget) {
            std::cerr << "shalimarc: --dsp-debug-index requires --target=tms6747\n";
            return 2;
        }
        if (mode != "--dump-ir" && mode != "--check" && mode != "--object" && mode != "-S" && mode != "--executable") {
            std::cerr << "shalimarc: supported modes are --check, --dump-ir and --object (S only)\n";
            return 2;
        }
        if (!liveRangeReportPath.empty() && mode != "--object") {
            std::cerr << "shalimarc: --debug-live-ranges requires --object\n"; return 2;
        }
        if (!variableReportPath.empty() && mode != "--object") {
            std::cerr << "shalimarc: --debug-variables requires --object\n"; return 2;
        }
        if (!debugMapPath.empty() && mode != "--object") {
            std::cerr << "shalimarc: --debug-map requires --object\n"; return 2;
        }


        if (diagnosticStages) std::cerr << "[S/debug] source=" << inputPath
                                        << " mode=" << (releaseMode ? "release" : "debug") << '\n';
        std::string source;
        if (!SourceReader().read(inputPath, source)) {
            std::cerr << "shalimarc: cannot read source file: " << inputPath << '\n';
            return 1;
        }

        std::vector<shalimar::Token> tokens;
        std::string error;
        if (!shalimar::Lexer().tokenize(source, tokens, error)) {
            std::cerr << inputPath << ": lex error: " << error << '\n';
            return 1;
        }

        shalimar::Program program;
        if (!shalimar::Parser(program).parse(tokens, program, error)) {
            std::cerr << inputPath << ": parse error: " << error << '\n';
            return 1;
        }
        if (!shalimar::SemanticChecker().check(program, error)) {
            std::cerr << inputPath << ": semantic error: " << error << '\n';
            return 1;
        }

        // S-mode IR must be structurally verified before and after optimization.
        if (dspTarget && (mode == "-S" || mode == "--executable" || mode == "--object")) {
            if (outputPath.empty()) { std::cerr << "shalimarc: DSP assembly output path required\n"; return 2; }
            shalimar::IRProgram ir;
            ir.sourceFile = inputPath;
#ifdef _WIN32
            char canonicalSource[4096];
            if (_fullpath(canonicalSource, inputPath.c_str(), sizeof(canonicalSource)))
                ir.sourceFile = canonicalSource;
#endif
            for (std::size_t i = 0; i < ir.sourceFile.size(); ++i)
                if (ir.sourceFile[i] == '\\') ir.sourceFile[i] = '/';
            if (!shalimar::IRBuilder().build(program, ir, error) ||
                !shalimar::IRVerifier().verify(ir, error)) {
                std::cerr << "shalimarc: DSP IR error: " << error << '\n'; return 1;
            }
            if (releaseMode) {
                const std::size_t before = instructionCount(ir);
                shalimar::IROptimizer().optimize(ir);
                if (optimizationReport) std::cerr << "[TMS6747/release] IR instructions: "
                    << before << " -> " << instructionCount(ir) << '\n';
                if (!shalimar::IRVerifier().verify(ir, error)) {
                    std::cerr << "shalimarc: DSP optimized IR error: " << error << '\n'; return 1;
                }
            }
            std::string assembly;
            const bool dspSymbols = debugSymbols || (!releaseMode && !ownDspTools);
            if (ownDspTools && debugSymbols) {
                std::cerr << "shalimarc: Shalimar ASM6x DWARF support is not implemented; use --toolchain=ti for source debugging\n"; return 1;
            }
            if (!shalimar::tms6747::IRAssembly().emit(ir, assembly, error, dspSymbols)) {
                std::cerr << "shalimarc: " << error << '\n'; return 1;
            }
            if (mode != "-S") {
                if (!shalimar::tms6747::Toolchain().build(ir, outputPath, ownDspTools ? toolsRoot : tiRoot, dspEntry,
                                                       releaseMode, dspSymbols, mode == "--object", error, ownDspTools, runtimePath)) {
                    std::cerr << "shalimarc: " << error << '\n'; return 1;
                }
                std::cout << "Generated TMS6747 " << (mode == "--object" ? "object: " : "executable: ") << outputPath << '\n';
                return 0;
            }
            if (!dspDebugIndexPath.empty()) {
                std::ofstream index(dspDebugIndexPath.c_str());
                if (!index) { std::cerr << "shalimarc: cannot create DSP debug index\n"; return 1; }
                index << "SHALIMAR_DSP_SOURCE_INDEX_V1\nsource\t" << ir.sourceFile << "\n";
                for (std::size_t fi = 0; fi < ir.functions.size(); ++fi) {
                    const shalimar::IRFunction& f = ir.functions[fi];
                    index << "function\t" << f.name << "\t" << f.debugLocation.line << "\n";
                }
            }
            std::ofstream out(outputPath.c_str(), std::ios::binary);
            if (!out || !(out << assembly)) { std::cerr << "shalimarc: cannot write DSP assembly\n"; return 1; }
            std::cout << "Generated experimental TMS6747 assembly: " << outputPath << '\n';
            return 0;
        }
        if (mode == "--dump-ir") {
            shalimar::IRProgram intermediate;
            intermediate.sourceFile = inputPath;
            if (!shalimar::IRBuilder().build(program, intermediate, error)) {
                std::cerr << "shalimarc: IR lowering error: " << error << '\n';
                return 1;
            }
            if (!shalimar::IRVerifier().verify(intermediate, error)) { std::cerr << "IR verification: " << error << "\n"; return 1; }
            if (releaseMode) {
                const std::size_t before = instructionCount(intermediate);
                shalimar::IROptimizer().optimize(intermediate);
                if (optimizationReport) std::cerr << "[S/release] IR instructions: "
                    << before << " -> " << instructionCount(intermediate) << '\n';
                if (!shalimar::IRVerifier().verify(intermediate, error)) { std::cerr << "optimized IR verification: " << error << "\n"; return 1; }
            }
            intermediate.dump(std::cout);
            return 0;
        }

        if (mode == "--check") {
            shalimar::IRProgram intermediate;
            intermediate.sourceFile = inputPath;
            if (!shalimar::IRBuilder().build(program, intermediate, error)) {
                std::cerr << "shalimarc: S-mode IR lowering error: " << error << '\n';
                return 1;
            }
            if (!shalimar::IRVerifier().verify(intermediate, error)) { std::cerr << "IR verification: " << error << "\n"; return 1; }
            if (releaseMode) { const std::size_t before = instructionCount(intermediate);
                shalimar::IROptimizer().optimize(intermediate);
                if (optimizationReport) std::cerr << "[S/release] IR instructions: "
                    << before << " -> " << instructionCount(intermediate) << '\n'; if (!shalimar::IRVerifier().verify(intermediate, error)) { std::cerr << "optimized IR verification: " << error << "\n"; return 1; } }
            std::cout << "S-mode frontend and IR lowering passed for " << selectedTarget
                      << " (not target code-generation or native debug verification).\n";
            return 0;
        }

        if (mode == "--object") {
            if (outputPath.empty()) { std::cerr << "shalimarc: object output path required\n"; return 2; }
            shalimar::IRProgram intermediate;
            intermediate.sourceFile = inputPath;
            if (!shalimar::IRBuilder().build(program, intermediate, error) ||
                !shalimar::IRVerifier().verify(intermediate, error)) {
                std::cerr << "shalimarc: S-mode IR error: " << error << '\n'; return 1;
            }
            if (releaseMode) {
                const std::size_t before = instructionCount(intermediate);
                shalimar::IROptimizer().optimize(intermediate);
                if (optimizationReport) std::cerr << "[S/release] IR instructions: "
                    << before << " -> " << instructionCount(intermediate) << '\n';
                if (!shalimar::IRVerifier().verify(intermediate, error)) {
                    std::cerr << "shalimarc: optimized IR error: " << error << '\n'; return 1;
                }
            }
            if (!intermediate.globals.empty()) {
                std::cerr << "shalimarc: S-mode COFF does not support global initializers yet\n"; return 1;
            }
            std::vector<shalimar::coff::FunctionBytes> functions;
            std::vector<std::vector<shalimar::x86_64::SourceMark> > debugMarks;
            shalimar::x86_64::IRSelector selector;
            for (std::size_t i = 0; i < intermediate.functions.size(); ++i) {
                shalimar::x86_64::InstructionEncoder encoder;
                shalimar::coff::FunctionBytes f;
                if (!selector.selectFunction(intermediate.functions[i], intermediate.functions, encoder, f.relocations, error, &f.debugVariables)) {
                    std::cerr << "shalimarc: S-mode COFF function " << intermediate.functions[i].name
                              << ": " << error << '\n'; return 1;
                }
                for (std::size_t r = 0; r < f.relocations.size(); ++r) {
                    bool validCallee = false;
                    for (std::size_t j = 0; j < intermediate.functions.size(); ++j) {
                        const shalimar::IRFunction& target = intermediate.functions[j];
                        if (target.name == f.relocations[r].symbol &&
                            target.signature.inputs.size() ==
                              (intermediate.functions[i].blocks[0].instructions[0].operand1.empty() ? 0U : 1U) &&
                            (target.signature.inputs.empty() ||
                             (target.signature.inputs.size() == 1 &&
                              target.signature.inputs[0].name == "int" &&
                              target.signature.inputs[0].arrayRank == 0)) &&
                            target.signature.outputs.size() == 1 &&
                            target.signature.outputs[0].name == "int" &&
                            target.signature.outputs[0].arrayRank == 0)
                            validCallee = true;
                    }
                    // Only a resolved imported sqrt(real)->real is currently callable.
                    if (!validCallee && f.relocations[r].symbol == "sqrt") {
                        for (std::size_t li = 0; li < intermediate.libraryImports.size(); ++li) {
                            const shalimar::IRLibraryImport& imp = intermediate.libraryImports[li];
                            if (imp.name == "sqrt" && imp.linkerSymbol == "sqrt" &&
                                imp.arity == 1 && imp.result.name == "real" &&
                                imp.arguments.size() == 1 && imp.arguments[0].name == "real") {
                                validCallee = true;
                            }
                        }
                    }
                    if (!validCallee) {
                        const std::vector<shalimar::IRInstruction>& callerIns = intermediate.functions[i].blocks[0].instructions;
                        for (std::size_t ci = 0; ci < callerIns.size(); ++ci) {
                            if (callerIns[ci].opcode == "call " + f.relocations[r].symbol &&
                                ci + 1 < callerIns.size() && callerIns[ci + 1].opcode == "multi.assign") {
                                for (std::size_t ti = 0; ti < intermediate.functions.size(); ++ti)
                                    if (intermediate.functions[ti].name == f.relocations[r].symbol &&
                                        intermediate.functions[ti].signature.inputs.empty() &&
                                        intermediate.functions[ti].signature.outputs.size() > 1)
                                        validCallee = true;
                            }
                        }
                    }
                    if (!validCallee) {
                        std::cerr << "shalimarc: unsupported or unresolved native call: "
                                  << f.relocations[r].symbol << '\n'; return 1;
                    }
                }
                f.name = intermediate.functions[i].name;
                f.code = encoder.bytes();
                if (debugSymbols) for (std::size_t mi = 0; mi < encoder.sourceMarks().size(); ++mi) {
                    const shalimar::x86_64::SourceMark& mark = encoder.sourceMarks()[mi];
                    shalimar::coff::DebugLine line;
                    line.offset = static_cast<unsigned>(mark.offset);
                    line.line = mark.line;
                    line.file = mark.file;
                    f.debugLines.push_back(line);
                }
                debugMarks.push_back(encoder.sourceMarks());
                functions.push_back(f);
            }
            if (!shalimar::coff::CoffWriter().write(functions, outputPath, error, debugSymbols)) {
                std::cerr << "shalimarc: COFF writer: " << error << '\n'; return 1;
            }
            // D4.2: auditable selector output. This report is NOT a PDB
            // variable-location claim; unknown storage remains unavailable.
            if (!variableReportPath.empty()) {
                std::ofstream report(variableReportPath.c_str(), std::ios::out | std::ios::trunc);
                if (!report) { std::cerr << "shalimarc: cannot open variable report\n"; return 1; }
                report << "SHALIMAR-DEBUG-VARIABLES\t1\n";
                for (std::size_t fi = 0; fi < functions.size(); ++fi) {
                    for (std::size_t vi = 0; vi < functions[fi].debugVariables.size(); ++vi) {
                        const shalimar::coff::DebugVariable& v = functions[fi].debugVariables[vi];
                        report << functions[fi].name << '\t' << v.name << '\t' << v.typeName << '\t';
                        if (v.location == shalimar::coff::DebugVariable::FrameRelative)
                            report << "frame\t" << v.registerCode << '\t' << v.frameOffset;
                        else if (v.location == shalimar::coff::DebugVariable::Register)
                            report << "register\t" << v.registerCode << "\t-";
                        else report << "unavailable\t-\t-";
                        report << '\n';
                    }
                }
                if (!report.good()) { std::cerr << "shalimarc: variable report write failed\n"; return 1; }
            }
            if (!liveRangeReportPath.empty()) {
                std::ofstream report(liveRangeReportPath.c_str(), std::ios::out | std::ios::trunc);
                if (!report) { std::cerr << "shalimarc: cannot open live-range report\n"; return 1; }
                report << "SHALIMAR-DEBUG-LIVE-RANGES\t1\n";
                for (std::size_t fi = 0; fi < functions.size(); ++fi)
                    for (std::size_t vi = 0; vi < functions[fi].debugVariables.size(); ++vi) {
                        const shalimar::coff::DebugVariable& v = functions[fi].debugVariables[vi];
                        report << functions[fi].name << '\t' << v.name << '\t';
                        if (v.location == shalimar::coff::DebugVariable::FrameRelative && v.hasLiveRange)
                            report << "frame\t" << v.frameOffset << '\t' << v.liveStart << '\t' << v.liveEnd;
                        else report << "unavailable\t-\t-\t-";
                        report << '\n';
                    }
                if (!report.good()) { std::cerr << "shalimarc: live-range report write failed\n"; return 1; }
            }
            if (!debugMapPath.empty()) {
                std::ofstream map(debugMapPath.c_str(), std::ios::out | std::ios::trunc);
                if (!map) { std::cerr << "shalimarc: cannot open debug map\n"; return 1; }
                map << "SHALIMAR-NATIVE-DEBUG-MAP\t1\n";
                std::size_t sectionOffset = 0;
                for (std::size_t fi = 0; fi < functions.size(); ++fi) {
                    for (std::size_t mi = 0; mi < debugMarks[fi].size(); ++mi) {
                        const shalimar::x86_64::SourceMark& mark = debugMarks[fi][mi];
                        if (mark.offset >= functions[fi].code.size() || mark.line == 0 || mark.file.empty()) {
                            std::cerr << "shalimarc: invalid native source mark\n"; return 1;
                        }
                        map << functions[fi].name << '\t' << sectionOffset + mark.offset
                            << '\t' << mark.file << '\t' << mark.line << '\t' << mark.column << '\n';
                    }
                    sectionOffset += functions[fi].code.size();
                }
                if (!map.good()) { std::cerr << "shalimarc: debug map write failed\n"; return 1; }
            }
            std::cout << "Generated restricted S-native Windows x64 COFF object: " << outputPath << '\n';
            return 0;
        }

        std::cerr << "shalimarc: unsupported native operation\n";
        return 2;
    }

private:
    static void printUsage() {
        std::cerr << "Shalimar S-native compiler (C++98)\n"
                  << "Usage: shalimarc input.shl --check [--debug|--release]\n"
                  << "       shalimarc input.shl --dump-ir [--debug|--release]\n"
                  << "       shalimarc input.shl --object output.obj [--debug|--release]\n"
                  << "       --middle=s is optional; C compilation is not supported.\n"
                  << "       Native targets: restricted Windows x64 COFF and TMS6747 scalar ELF.\n"
                  << "       --target=windows-x86_64|linux-x86_64|macos-arm64|tms6747\n"
                  << "       Linux/macOS currently support --check and --dump-ir only.\n"
                  << "       --diagnostic-stages logs compilation mode/source.\n"
                  << "       --optimization-report reports Release IR instruction counts.\n"
                  << "       input.shl -S output.asm --target=tms6747 [--debug|--release] [--dsp-debug-index PATH] (restricted)\n"
                  << "       input.shl --target=tms6747 -o output.out [--debug|--release] [--debug-symbols]\n"
                  << "       DSP options: --ti-root PATH, --dsp-entry NAME (default main), --object output.obj\n"
                  << "       DSP toolchain: --toolchain=shalimar (default), --tools-root PATH, --rts-lib PATH\n"
                  << "       --toolchain=ti selects the separate TI oracle tools and DWARF debugging.\n"
                  << "       --debug-map PATH writes provisional native source offsets (separate from CodeView).\n"
                  << "       DSP DWARF supports source lines and scalar stack variables; target coverage is incomplete.\n";
    }

};

} // namespace

int main(int argc, char** argv) {
    try {
        return CompilerDriver().run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "shalimarc: compiler failure: " << error.what() << '\n';
        return 1;
    }
}
