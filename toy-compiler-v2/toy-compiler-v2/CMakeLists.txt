cmake_minimum_required(VERSION 3.16)
project(toy-compiler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(toy-compiler
    src/main.cpp
    src/lexing/token.cpp
    src/lexing/lexer.cpp
    src/parsing/ast.cpp
    src/parsing/parser.cpp
    src/semantic/semantic.cpp
    src/optimizer/optimizer.cpp
    src/codegen/arm64.cpp
)

target_compile_options(toy-compiler PRIVATE -Wall -Wextra)
