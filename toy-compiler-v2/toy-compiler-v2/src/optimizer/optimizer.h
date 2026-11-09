#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../parsing/ast.h"

// a value we know at compile time
struct ConstValue {
    enum class Kind { Int, String, Bool };

    Kind kind;
    int32_t intValue = 0;
    std::string stringValue;
    bool boolValue = false;
};

class Optimizer {
public:
    std::vector<StmtPtr> optimize(std::vector<StmtPtr> stmts);

private:
    std::unordered_map<std::string, ConstValue> constants;
    std::unordered_set<std::string> usedVars;

    // statements
    std::vector<StmtPtr> optimizeStmts(const std::vector<StmtPtr>& stmts);
    std::vector<StmtPtr> optimizeStmt(const StmtPtr& stmt);
    std::vector<StmtPtr> optimizeIf(const StmtPtr& stmt);

    // expressions
    ExprPtr optimizeExpr(const ExprPtr& expr);
    ExprPtr optimizeBinary(const ExprPtr& expr);

    // constant folding
    std::optional<ConstValue> evalConst(const ExprPtr& expr) const;
    ExprPtr fold(const ConstValue& l, BinOp op, const ConstValue& r) const; // nullptr if it can't

    // dead code
    void collectUsedVars(const std::vector<StmtPtr>& stmts);
    void collectStmt(const StmtPtr& stmt);
    void collectExpr(const ExprPtr& expr);
    std::vector<StmtPtr> deadCodeElimination(const std::vector<StmtPtr>& stmts) const;
};
