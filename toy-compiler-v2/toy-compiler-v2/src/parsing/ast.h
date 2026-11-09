#pragma once

// the ast is two kinds of nodes: expressions (things that make a value)
// and statements (things that do something).
//
// expressions are literals, identifiers, `maybe`, binary ops like a + b,
// and assignments.
// statements are print, variable declarations, if/else, blocks,
// expression statements and paywall.
//
// an if holds its condition (an expression), the "then" statements to run
// when it's true, and optionally the "else" statements for when it isn't.
//
// nodes are shared_ptrs and never get changed after they're made.
// the optimizer always builds new nodes, so copying a tree is cheap.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class BinOp {
    Add,
    Sub,
    GreaterThan,
    LessThan,
};

enum class ExprKind {
    IntegerLiteral,
    StringLiteral,
    BooleanLiteral,
    Identifier,
    Maybe,
    Binary,
    Assign,
};

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
    ExprKind kind;

    int32_t intValue = 0;   // IntegerLiteral
    bool boolValue = false; // BooleanLiteral
    std::string text;       // StringLiteral value, or variable name for Identifier / Assign

    BinOp op = BinOp::Add;  // Binary
    ExprPtr left;           // Binary
    ExprPtr right;          // Binary
    ExprPtr value;          // Assign
};

enum class StmtKind {
    Print,
    VarDeclaration,
    If,
    Block,
    ExprStmt,
    Paywall,
};

struct Stmt;
using StmtPtr = std::shared_ptr<Stmt>;

struct Stmt {
    StmtKind kind;

    std::string name;           // VarDeclaration
    ExprPtr expr;               // Print / ExprStmt / VarDeclaration value / If condition

    std::vector<StmtPtr> body;  // Block contents, or the "then" part of an If
    bool hasElse = false;
    std::vector<StmtPtr> elseBody;

    int64_t amount = 0;         // Paywall
};

// ---------- little helpers for making nodes ----------

ExprPtr makeInt(int32_t n);
ExprPtr makeString(const std::string& s);
ExprPtr makeBool(bool b);
ExprPtr makeIdentifier(const std::string& name);
ExprPtr makeMaybe();
ExprPtr makeBinary(ExprPtr left, BinOp op, ExprPtr right);
ExprPtr makeAssign(const std::string& name, ExprPtr value);

StmtPtr makePrint(ExprPtr e);
StmtPtr makeVarDecl(const std::string& name, ExprPtr value);
StmtPtr makeIf(ExprPtr cond, std::vector<StmtPtr> thenBody, bool hasElse, std::vector<StmtPtr> elseBody);
StmtPtr makeBlock(std::vector<StmtPtr> stmts);
StmtPtr makeExprStmt(ExprPtr e);
StmtPtr makePaywall(int64_t amount);

// deep comparison, the optimizer uses this to know when to stop
bool sameExpr(const ExprPtr& a, const ExprPtr& b);
bool sameStmt(const StmtPtr& a, const StmtPtr& b);
bool sameStmts(const std::vector<StmtPtr>& a, const std::vector<StmtPtr>& b);
