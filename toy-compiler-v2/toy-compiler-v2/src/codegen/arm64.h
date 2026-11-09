#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../parsing/ast.h"

// turns the (already checked + optimized) ast into arm64 assembly text
class Codegen {
public:
    std::string generate(const std::vector<StmtPtr>& stmts);

private:
    std::string out;
    std::unordered_map<std::string, size_t> vars; // variable name -> stack offset
    size_t stackOffset = 0;
    int labelCounter = 0;

    void genStmt(const StmtPtr& stmt);
    std::string genExpr(const ExprPtr& expr); // returns the register holding the result

    std::string allocTmp();
    std::string label(const std::string& prefix);
    void emit(const std::string& line);
};
