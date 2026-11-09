# toy compiler v2 (c++)
a small compiler with explicit architecture

toy compiler is a small compiler written in c++ that compiles a simple c-like
language into arm64 assembly, assembles it with aarch64-linux-gnu-gcc,
and runs it through qemu.

it's a learning project. each phase is its own folder and passes a plain
data structure to the next one, nothing fancy.

(this started as a rust project, i ported it to c++.)

## project structure
```
src/
├── lexing/      # regex-based tokenization
├── parsing/     # ast + recursive descent parser
├── semantic/    # scopes and type checking
├── optimizer/   # ast-to-ast passes
├── codegen/     # arm64 assembly generation
├── asm/         # hand-written arm64 i used to learn the basics
└── main.cpp     # driver, runs each phase and times it
```

pipeline:
source -> lexer -> parser -> semantic -> optimizer -> codegen -> gcc -> qemu

## language

```
int x = 10;

if (maybe) {          // maybe = coin flip at runtime
    print(x);
} else {
    print(x - 1);
}

paywall(200);         // asks you to type 'haha' or the program exits
```

- types: int, string, bool (from comparisons and `maybe`)
- operators: `+ - < >`
- statements: `int` declarations, `print`, `if / else`, blocks, `paywall`

## semantic rules
- variables must be declared before use (if not, it asks if it should just pretend it's an int)
- no redeclaring in the same scope
- `int` variables need int values
- if conditions must be bool
- errors are collected and printed together

## optimizer
runs passes until nothing changes (max 10):
- constant folding
- constant propagation
- identity simplification (`x + 0`, `x - 0`)
- if folding (drops the branch that can't run)
- removing unused variables

## codegen
- target: arm64
- every variable gets an 8 byte stack slot
- bools are 0 / 1
- printf / scanf / strcmp / rand from libc

known issue: there's only one temp register (x14) right now, so
expressions with two non-constant sides like `a + b` don't compute right yet.

## running
requirements:
- a c++17 compiler + cmake
- aarch64-linux-gnu-gcc
- qemu-aarch64

```
cmake -S . -B build
cmake --build build
./build/toy-compiler
```

the test program is in `makeProgram()` in main.cpp. flip the `true`
to `false` for the normal (non chaos) one.

generated files:
- out.s   arm64 assembly
- out     executable

