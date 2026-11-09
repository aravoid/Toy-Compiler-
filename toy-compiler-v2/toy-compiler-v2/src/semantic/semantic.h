#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../parsing/ast.h"

enum class Type {
    Int,
    String,
    Bool,
    Unknown,
};

std::string typeName(Type t);

struct SemanticError {
    enum class Kind { UndeclaredVariable, Redeclaration, TypeMismatch };

    Kind kind;
    std::string name;          // UndeclaredVariable / Redeclaration
    Type expected = Type::Unknown;
    Type found = Type::Unknown;
    std::string context;       // TypeMismatch

    std::string toString() const;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    // returns every error found. empty means the program is fine
    std::vector<SemanticError> analyze(const std::vector<StmtPtr>& stmts);

private:
    // one map per scope, the innermost scope is at the back
    std::vector<std::unordered_map<std::string, Type>> scopes;
    std::vector<SemanticError> errors;

    // scopes
    void enterScope();
    void exitScope();
    std::unordered_map<std::string, Type>& currentScope();
    bool lookup(const std::string& name, Type& out) const;

    void error(SemanticError err);

    // statements
    void checkStmt(const StmtPtr& stmt);
    void checkVarDecl(const std::string& name, const ExprPtr& value);
    void checkIf(const StmtPtr& stmt);

    // expressions
    Type checkExpr(const ExprPtr& expr);
    Type checkBinary(const ExprPtr& expr);
};
