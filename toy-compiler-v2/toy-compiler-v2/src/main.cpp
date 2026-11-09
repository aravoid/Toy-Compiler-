#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "codegen/arm64.h"
#include "lexing/lexer.h"
#include "optimizer/optimizer.h"
#include "parsing/parser.h"
#include "semantic/semantic.h"

using Clock = std::chrono::steady_clock;

std::string makeProgram(int repetitions, bool chaosMode) {
    std::string standardBlock = R"(
    {
       int x = 69;
       int y = 420;
       if (x > y) {
            print(x + y);
        } else {
            print(y - x);
        }
    }
    )";

    std::string chaosBlock = R"(
    {
        if (maybe) {
            print(hihi);
        } else {
            print(69);
        }
    }
    paywall(200);
    print(999999);
    )";

    std::string block = chaosMode ? chaosBlock : standardBlock;
    std::string program;
    for (int i = 0; i < repetitions; i++) {
        program += block;
    }
    return program;
}

// prints a duration like 15.2ms / 30.5µs
std::string formatDuration(Clock::duration d) {
    double ns = std::chrono::duration<double, std::nano>(d).count();
    std::ostringstream ss;
    if (ns >= 1e9) {
        ss << ns / 1e9 << "s";
    } else if (ns >= 1e6) {
        ss << ns / 1e6 << "ms";
    } else if (ns >= 1e3) {
        ss << ns / 1e3 << "µs";
    } else {
        ss << ns << "ns";
    }
    return ss.str();
}

int main() {
    // config:
    int repetitions = 1; // try: 1, 10, 50, 100, 500
    std::string program = makeProgram(repetitions, true);

    std::cout << "benchmarking with " << repetitions << " repeated blocks\n";

    auto totalStart = Clock::now();

    // lexing:
    auto lexStart = Clock::now();
    std::vector<Token> tokens = lexProgram(program);
    auto lexTime = Clock::now() - lexStart;

    // parsing:
    auto parseStart = Clock::now();
    std::vector<StmtPtr> ast;
    try {
        Parser parser(tokens);
        ast = parser.parse();
    } catch (const ParseError& e) {
        std::cout << "parse error: " << e.what() << "\n";
        return 0;
    }
    auto parseTime = Clock::now() - parseStart;

    // semantic
    auto semanticStart = Clock::now();
    SemanticAnalyzer analyzer;
    std::vector<SemanticError> errors = analyzer.analyze(ast);
    auto semanticTime = Clock::now() - semanticStart;

    if (!errors.empty()) {
        std::cout << "semantic errors (" << errors.size() << "):\n";
        for (const auto& e : errors) {
            std::cout << "  " << e.toString() << "\n";
        }
    }

    // optimizer
    auto optimizeStart = Clock::now();
    Optimizer optimizer;
    std::vector<StmtPtr> optimizedAst = optimizer.optimize(ast);
    auto optimizeTime = Clock::now() - optimizeStart;

    // codegen
    auto codegenStart = Clock::now();
    Codegen codegen;
    std::string asmText = codegen.generate(optimizedAst);
    auto codegenTime = Clock::now() - codegenStart;

    std::ofstream("out.s") << asmText;

    std::cout << "\n generated assembly: \n\n";

    // prevent terminal nuking on huge outputs
    const int maxLines = 300;
    std::istringstream lines(asmText);
    std::string line;
    int lineCount = 0;
    int totalLines = 0;
    for (char c : asmText) {
        if (c == '\n') totalLines++;
    }
    while (std::getline(lines, line)) {
        if (lineCount >= maxLines) {
            std::cout << "... (assembly truncated, " << totalLines << "+ lines total)\n";
            break;
        }
        std::cout << line << "\n";
        lineCount++;
    }

    // assemble + link with the cross compiler
    auto assembleStart = Clock::now();
    int assembleStatus = std::system("aarch64-linux-gnu-gcc -static out.s -o out");
    auto assembleTime = Clock::now() - assembleStart;

    if (assembleStatus != 0) {
        std::cout << "assembly failed\n";
        std::cout << "\n timings: \n";
        std::cout << "lexing:        " << formatDuration(lexTime) << "\n";
        std::cout << "parsing:       " << formatDuration(parseTime) << "\n";
        std::cout << "semantic:      " << formatDuration(semanticTime) << "\n";
        std::cout << "codegen:       " << formatDuration(codegenTime) << "\n";
        std::cout << "assemble:      FAILED\n";
        std::cout << "total:         " << formatDuration(Clock::now() - totalStart) << "\n";
        return 0;
    }

    // ================= RUNTIME =================
    auto runStart = Clock::now();
    int runStatus = std::system("qemu-aarch64 ./out");
    auto runTime = Clock::now() - runStart;

    if (runStatus != 0) {
        std::cout << "runtime execution failed\n";
    }

    // ================= TIMINGS =================
    std::cout << "\n--- TIMINGS ---\n";
    std::cout << "Lexing:        " << formatDuration(lexTime) << "\n";
    std::cout << "Parsing:       " << formatDuration(parseTime) << "\n";
    std::cout << "Semantic:      " << formatDuration(semanticTime) << "\n";
    std::cout << "Optimizer:     " << formatDuration(optimizeTime) << "\n";
    std::cout << "Codegen:       " << formatDuration(codegenTime) << "\n";
    std::cout << "Assemble:      " << formatDuration(assembleTime) << "\n";
    std::cout << "Runtime:       " << formatDuration(runTime) << "\n";
    std::cout << "Total:         " << formatDuration(Clock::now() - totalStart) << "\n";

    std::cout << "\nexecutable: out.exe\n";
    return 0;
}
