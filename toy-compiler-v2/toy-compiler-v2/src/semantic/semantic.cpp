#include "semantic.h"

#include <iostream>

std::string typeName(Type t) {
    switch (t) {
        case Type::Int:     return "Int";
        case Type::String:  return "String";
        case Type::Bool:    return "Bool";
        case Type::Unknown: return "Unknown";
    }
    return "?";
}

std::string SemanticError::toString() const {
    switch (kind) {
        case Kind::UndeclaredVariable:
            return "UndeclaredVariable(\"" + name + "\")";
        case Kind::Redeclaration:
            return "Redeclaration(\"" + name + "\")";
        case Kind::TypeMismatch:
            return "TypeMismatch { expected: " + typeName(expected) + ", found: " + typeName(found) +
                   ", context: \"" + context + "\" }";
    }
    return "?";
}

// small helpers so the checks below read better
static SemanticError undeclared(const std::string& name) {
    SemanticError e;
    e.kind = SemanticError::Kind::UndeclaredVariable;
    e.name = name;
    return e;
}

static SemanticError redeclared(const std::string& name) {
    SemanticError e;
    e.kind = SemanticError::Kind::Redeclaration;
    e.name = name;
    return e;
}

static SemanticError mismatch(Type expected, Type found, const std::string& context) {
    SemanticError e;
    e.kind = SemanticError::Kind::TypeMismatch;
    e.expected = expected;
    e.found = found;
    e.context = context;
    return e;
}

SemanticAnalyzer::SemanticAnalyzer() {
    scopes.push_back({}); // global scope
}

// ---------- scopes ----------

void SemanticAnalyzer::enterScope() {
    scopes.push_back({});
}

void SemanticAnalyzer::exitScope() {
    scopes.pop_back();
}

std::unordered_map<std::string, Type>& SemanticAnalyzer::currentScope() {
    return scopes.back();
}

// search from the innermost scope outwards
bool SemanticAnalyzer::lookup(const std::string& name, Type& out) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            out = found->second;
            return true;
        }
    }
    return false;
}

void SemanticAnalyzer::error(SemanticError err) {
    errors.push_back(err);
}

// ---------- entry ----------

std::vector<SemanticError> SemanticAnalyzer::analyze(const std::vector<StmtPtr>& stmts) {
    for (const auto& s : stmts) {
        checkStmt(s);
    }
    return errors;
}

// ---------- statements ----------

void SemanticAnalyzer::checkStmt(const StmtPtr& stmt) {
    switch (stmt->kind) {
        case StmtKind::Block:
            enterScope();
            for (const auto& s : stmt->body) {
                checkStmt(s);
            }
            exitScope();
            break;

        case StmtKind::VarDeclaration:
            checkVarDecl(stmt->name, stmt->expr);
            break;

        case StmtKind::Print:
        case StmtKind::ExprStmt:
            checkExpr(stmt->expr);
            break;

        case StmtKind::If:
            checkIf(stmt);
            break;

        case StmtKind::Paywall:
            break; // nothing to check
    }
}

void SemanticAnalyzer::checkVarDecl(const std::string& name, const ExprPtr& value) {
    if (currentScope().count(name)) {
        error(redeclared(name));
        return;
    }

    Type valueType = checkExpr(value);

    if (valueType != Type::Int && valueType != Type::Unknown) {
        error(mismatch(Type::Int, valueType, "initializer for '" + name + "' must be Int"));
    }

    currentScope()[name] = Type::Int;
}

void SemanticAnalyzer::checkIf(const StmtPtr& stmt) {
    Type condType = checkExpr(stmt->expr);

    if (condType != Type::Bool && condType != Type::Unknown) {
        error(mismatch(Type::Bool, condType, "if condition must be boolean"));
    }

    enterScope();
    for (const auto& s : stmt->body) {
        checkStmt(s);
    }
    exitScope();

    if (stmt->hasElse) {
        enterScope();
        for (const auto& s : stmt->elseBody) {
            checkStmt(s);
        }
        exitScope();
    }
}

// ---------- expressions ----------

Type SemanticAnalyzer::checkExpr(const ExprPtr& expr) {
    switch (expr->kind) {
        case ExprKind::IntegerLiteral: return Type::Int;
        case ExprKind::StringLiteral:  return Type::String;
        case ExprKind::BooleanLiteral: return Type::Bool;
        case ExprKind::Maybe:          return Type::Bool;

        case ExprKind::Identifier: {
            Type t;
            if (lookup(expr->text, t)) {
                return t;
            }

            // undefined variable, ask the user what to do
            std::cout << "wait, variable " << expr->text << " is undefined. \n"
                      << " should i pretend its an INT? (y/n:)" << std::endl;

            std::string input;
            std::getline(std::cin, input);

            // trim whitespace
            size_t first = input.find_first_not_of(" \t\r\n");
            size_t last = input.find_last_not_of(" \t\r\n");
            input = (first == std::string::npos) ? "" : input.substr(first, last - first + 1);

            if (input == "y") {
                std::cout << "okay, i'll treat " << expr->text << " as an INT for now" << std::endl;
                currentScope()[expr->text] = Type::Int;
                return Type::Int;
            }

            error(undeclared(expr->text));
            return Type::Unknown;
        }

        case ExprKind::Assign: {
            Type varType;
            if (!lookup(expr->text, varType)) {
                error(undeclared(expr->text));
                varType = Type::Unknown;
            }

            Type valueType = checkExpr(expr->value);

            if (varType != Type::Unknown && valueType != Type::Unknown && varType != valueType) {
                error(mismatch(varType, valueType, "cannot assign to '" + expr->text + "'"));
            }

            return varType;
        }

        case ExprKind::Binary:
            return checkBinary(expr);
    }
    return Type::Unknown;
}

Type SemanticAnalyzer::checkBinary(const ExprPtr& expr) {
    Type lt = checkExpr(expr->left);
    Type rt = checkExpr(expr->right);

    switch (expr->op) {
        case BinOp::Add:
            if (lt == Type::Int && rt == Type::Int) return Type::Int;
            if (lt == Type::String && rt == Type::String) return Type::String;
            error(mismatch(lt, rt, "invalid '+' operands"));
            return Type::Unknown;

        case BinOp::Sub:
            if (lt == Type::Int && rt == Type::Int) return Type::Int;
            error(mismatch(Type::Int, lt != Type::Int ? lt : rt, "subtraction requires Int"));
            return Type::Unknown;

        case BinOp::GreaterThan:
        case BinOp::LessThan:
            if (lt == Type::Int && rt == Type::Int) return Type::Bool;
            error(mismatch(Type::Int, lt != Type::Int ? lt : rt, "comparison requires Int"));
            return Type::Unknown;
    }
    return Type::Unknown;
}
