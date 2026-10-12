cmake_minimum_required(VERSION 3.10)

project(ShalimarCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    shalimarc
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/SemanticChecker.cpp
    src/IRVerifier.cpp
    src/IRBuilder.cpp
    src/IROptimizer.cpp
    src/backend/coff/CoffWriter.cpp
    src/backend/common/NativeBackend.cpp
    src/backend/x86_64/InstructionEncoder.cpp
    src/backend/x86_64/IRSelector.cpp
    src/backend/tms6747/TargetContract.cpp
    src/backend/tms6747/IRAssembly.cpp
    src/backend/tms6747/Toolchain.cpp
    src/backend/legacy_port/Arm64Darwin.cpp
    src/backend/legacy_port/Emitter.cpp
    src/backend/legacy_port/Optimize.cpp
    src/backend/legacy_port/Spelling.cpp
    src/backend/legacy_port/Tms6747.cpp
    src/backend/legacy_port/X86_64.cpp
    src/backend/legacy_port/X86_64Linux.cpp
    src/backend/legacy_port/X86_64Windows.cpp
)

target_include_directories(shalimarc PRIVATE include)

if(MSVC)
    target_compile_options(shalimarc PRIVATE /W4)
else()
    target_compile_options(shalimarc PRIVATE -Wall -Wextra -pedantic)
endif()
