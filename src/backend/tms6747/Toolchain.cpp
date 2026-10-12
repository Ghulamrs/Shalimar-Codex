#include "backend/tms6747/Toolchain.h"
#include "backend/tms6747/IRAssembly.h"
#include "../legacy_port/Tms6747.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace shalimar { namespace tms6747 {
namespace {
bool writeFile(const std::string& path, const std::string& value) {
    std::ofstream stream(path.c_str(), std::ios::binary);
    return stream && (stream << value) && stream.flush();
}
bool readable(const std::string& path) { std::ifstream file(path.c_str(), std::ios::binary); return !!file; }
std::string ownExecutable(const std::string& root, const std::string& name) {
    return readable(root + "/bin/" + name) ? root + "/bin/" + name : root + "/" + name;
}
std::string quote(const std::string& value) {
    std::string result = "\"";
    unsigned slashes = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\') { ++slashes; continue; }
        result.append(slashes * (value[i] == '"' ? 2 : 1), '\\');
        slashes = 0;
        if (value[i] == '"') result += '\\';
        result += value[i];
    }
    result.append(slashes * 2, '\\');
    return result + "\"";
}
bool run(const std::string& executable, const std::vector<std::string>& arguments,
         const std::string& log, std::string& error) {
#ifdef _WIN32
    std::string command = quote(executable);
    for (std::size_t i = 0; i < arguments.size(); ++i) command += " " + quote(arguments[i]);
    std::vector<char> buffer(command.begin(), command.end()); buffer.push_back(0);
    SECURITY_ATTRIBUTES security = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    HANDLE file = CreateFileA(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) { error = "Cannot create target-tool diagnostic log"; return false; }
    STARTUPINFOA startup; ZeroMemory(&startup, sizeof(startup)); startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = file; startup.hStdError = file; startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process; ZeroMemory(&process, sizeof(process));
    BOOL started = CreateProcessA(executable.c_str(), &buffer[0], 0, 0, TRUE,
                                  CREATE_NO_WINDOW, 0, 0, &startup, &process);
    if (!started) {
        std::ostringstream detail; detail << "Cannot launch target tool (Windows error " << GetLastError() << "); log: " << log;
        error = detail.str(); CloseHandle(file); return false;
    }
    DWORD wait = WaitForSingleObject(process.hProcess, 120000);
    DWORD code = 1;
    if (wait == WAIT_TIMEOUT) { TerminateProcess(process.hProcess, 124); WaitForSingleObject(process.hProcess, 5000); }
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread); CloseHandle(process.hProcess); CloseHandle(file);
    if (code != 0 || wait != WAIT_OBJECT_0) {
        std::ostringstream detail; detail << "Target tool failed (exit " << code << "); diagnostics: " << log;
        error = detail.str(); return false;
    }
    return true;
#else
    (void)executable; (void)arguments; (void)log;
    error = "Integrated TI tool launch currently requires a Windows host; use -S elsewhere";
    return false;
#endif
}
}
bool Toolchain::build(const IRProgram& input, const std::string& output,
                      const std::string& root, const std::string& entry,
                      bool release, bool symbols, bool objectOnly, std::string& error,
                      bool ownTools, const std::string& runtime) const {
    if (ownTools && symbols) { error = "Shalimar ASM6x does not support DWARF directives yet; use --toolchain=ti for source-debugger qualification"; return false; }
    IRProgram program = input;
    std::string entrySymbol;
    bool hasOutput = false;
    bool realOutput = false;
    if (!objectOnly) {
        for (std::size_t i = 0; i < program.functions.size(); ++i) {
            IRFunction& function = program.functions[i];
            if (function.name == entry) {
                if (!function.signature.inputs.empty() || function.signature.outputs.size() > 1 ||
                    (!function.signature.outputs.empty() &&
                     ((function.signature.outputs[0].name != "int" && function.signature.outputs[0].name != "real") || function.signature.outputs[0].arrayRank))) {
                    error = "DSP entry must take no arguments and return void, int or real"; return false;
                }
                hasOutput = !function.signature.outputs.empty();
                realOutput = hasOutput && function.signature.outputs[0].name == "real";
                entrySymbol = entry == "main" ? "shalimar_main" : entry;
            }
            if (function.name == "main") function.name = "shalimar_main";
        }
        if (entrySymbol.empty()) { error = "DSP entry function not found: " + entry; return false; }
        for (std::size_t i = 0; i < program.functions.size(); ++i)
            for (std::size_t b = 0; b < program.functions[i].blocks.size(); ++b)
                for (std::size_t n = 0; n < program.functions[i].blocks[b].instructions.size(); ++n) {
                    IRInstruction& instruction = program.functions[i].blocks[b].instructions[n];
                    if (instruction.opcode == "call main") instruction.opcode = "call shalimar_main";
                }
    }
    std::string assembly;
    if (!IRAssembly().emit(program, assembly, error, symbols)) return false;
    const std::string asmFile = output + ".asm";
    if (!writeFile(asmFile, assembly)) { error = "Cannot write DSP assembly"; return false; }
    std::vector<std::string> args;
    args.push_back("-mv6740"); args.push_back("--abi=eabi");
    if (symbols) args.push_back("-g");
    // Explicit per-input object files avoid current-directory side effects.
    args.push_back("--compile_only"); args.push_back("--output_file=" + output + ".code.obj"); args.push_back(asmFile);
    const std::string compiler = root + "/bin/cl6x.exe";
    if (ownTools) {
        args.clear(); args.push_back(asmFile); args.push_back("-o"); args.push_back(output + ".code.obj");
    }
    if (!run(ownTools ? ownExecutable(root, "asm6x.exe") : compiler, args, output + ".assemble.log", error)) return false;
    if (objectOnly) {
        std::ifstream in((output + ".code.obj").c_str(), std::ios::binary);
        std::ofstream out(output.c_str(), std::ios::binary);
        if (!in || !out || !(out << in.rdbuf())) { error = "Cannot write DSP object"; return false; }
        return true;
    }
    if (ownTools) {
        std::string library = runtime;
        if (library.empty()) library = readable(root + "/lib/rts6x.lib") ? root + "/lib/rts6x.lib" : root + "/rts6x.lib";
        if (!readable(library)) { error = "Own RTS6x library missing: " + library + "; specify --rts-lib PATH"; return false; }
        Tms6747Emitter startup; startup.configureNativeDebug(false, output + ".startup.s");
        startup.beginModule(output + ".startup.s"); startup.beginFunction("main"); startup.call(entrySymbol);
        if (!hasOutput || realOutput) {
            if (realOutput) {
                startup.nativeInstruction("MVKL probe_real, A3"); startup.nativeInstruction("MVKH probe_real, A3");
                startup.nativeInstruction("STDW A5:A4, *A3");
            }
            startup.loadIntConstant(0);
        }
        startup.nativeInstruction("MVKL probe_result, A3"); startup.nativeInstruction("MVKH probe_result, A3");
        startup.nativeInstruction("STW A4, *A3"); startup.storeSlot(Int, 0);
        startup.call("halt_here"); startup.loadSlot(Int, 0); startup.endFunction(1);
        startup.beginFunction("halt_here"); startup.endFunction(0);
        startup.nativeInstruction(".global probe_result"); startup.nativeInstruction(".bss probe_result, 4, 4");
        startup.nativeInstruction(".global probe_real"); startup.nativeInstruction(".bss probe_real, 8, 8");
        startup.endModule();
        if (!writeFile(output + ".startup.s", startup.text())) { error = "Cannot write own-tool startup assembly"; return false; }
        args.clear(); args.push_back(output + ".startup.s"); args.push_back("-o"); args.push_back(output + ".startup.obj");
        if (!run(ownExecutable(root, "asm6x.exe"), args, output + ".startup.log", error)) return false;
        const std::string layout = "--stack_size=0x4000\n--heap_size=0x4000\n"
            "MEMORY { L2RAM : origin=0x11800000, length=0x40000 }\n"
            "SECTIONS { .text > L2RAM .stack > L2RAM .bss > L2RAM .neardata > L2RAM "
            ".rodata > L2RAM .data > L2RAM .const > L2RAM .cinit > L2RAM .pinit > L2RAM "
            ".init_array > L2RAM .far > L2RAM .fardata > L2RAM .cio > L2RAM .sysmem > L2RAM "
            ".switch > L2RAM .c6xabi.exidx > L2RAM .c6xabi.extab > L2RAM }\n";
        if (!writeFile(output + ".cmd", layout)) { error = "Cannot write own-tool linker layout"; return false; }
        args.clear(); args.push_back(output + ".cmd"); args.push_back(output + ".code.obj"); args.push_back(output + ".startup.obj");
        args.push_back(library); args.push_back("--rom_model"); args.push_back("-e"); args.push_back("_c_int00");
        args.push_back("-o"); args.push_back(output); args.push_back("-m"); args.push_back(output + ".map");
        return run(ownExecutable(root, "lnk6x.exe"), args, output + ".link.log", error);
    }
    std::ostringstream startup;
    startup << "extern " << (realOutput ? "double" : hasOutput ? "int" : "void") << " " << entrySymbol << "(void);\n"
            << "volatile int probe_result = -1;\n"
            << "volatile double probe_real = 0.0;\n"
            << "void halt_here(void) { for (;;) {} }\n"
            << "int main(void) { ";
    if (realOutput) startup << "probe_real = " << entrySymbol << "(); probe_result = 0;";
    else if (hasOutput) startup << "probe_result = " << entrySymbol << "();";
    else startup << entrySymbol << "(); probe_result = 0;";
    startup << " halt_here(); return 0; }\n";
    if (!writeFile(output + ".startup.c", startup.str())) { error = "Cannot write DSP startup harness"; return false; }
    args.clear(); args.push_back("-mv6740"); args.push_back("--abi=eabi"); args.push_back("-g");
    args.push_back(release ? "-O2" : "-O0"); args.push_back("--compile_only");
    args.push_back("--output_file=" + output + ".startup.obj"); args.push_back(output + ".startup.c");
    if (!run(compiler, args, output + ".startup.log", error)) return false;
    const std::string layout =
        "--stack_size=0x4000\n--heap_size=0x4000\n"
        "MEMORY { L2RAM : origin=0x11800000, length=0x40000 }\n"
        "SECTIONS { .text > L2RAM .stack > L2RAM .bss > L2RAM .neardata > L2RAM "
        ".rodata > L2RAM .data > L2RAM .const > L2RAM .cinit > L2RAM "
        ".pinit > L2RAM .init_array > L2RAM .far > L2RAM .fardata > L2RAM "
        ".cio > L2RAM .sysmem > L2RAM .switch > L2RAM .c6xabi.exidx > L2RAM .c6xabi.extab > L2RAM }\n";
    if (!writeFile(output + ".cmd", layout)) { error = "Cannot write DSP memory layout"; return false; }
    args.clear(); args.push_back("-mv6740"); args.push_back("--abi=eabi"); args.push_back("--run_linker");
    args.push_back("--rom_model"); args.push_back("--search_path=" + root + "/lib");
    args.push_back("--output_file=" + output); args.push_back("--map_file=" + output + ".map");
    args.push_back(output + ".code.obj"); args.push_back(output + ".startup.obj");
    args.push_back(output + ".cmd"); args.push_back("--library=rts6740_elf.lib");
    return run(compiler, args, output + ".link.log", error);
}
}}
