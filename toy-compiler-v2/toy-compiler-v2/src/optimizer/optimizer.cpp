#include "optimizer.h"

// -------- ENTRY --------

std::vector<StmtPtr> Optimizer::optimize(std::vector<StmtPtr> stmts) {
    std::vector<StmtPtr> current = stmts;

    // run up to 10 passes to catch nested optimizations
    for (int pass = 0; pass < 10; pass++) {
        constants.clear();
        usedVars.clear();

        // pass 1: analyze usage
        collectUsedVars(current);

        // pass 2: optimize (fold constants)
        std::vector<StmtPtr> optimized = optimizeStmts(current);

        // pass 3: clean (remove unused variables)
        std::vector<StmtPtr> cleaned = deadCodeElimination(optimized);

        if (sameStmts(optimized, current)) {
            break; // nothing changed, we're done
        }

        current = cleaned;
    }

    return current;
}

// -------- STATEMENTS --------

std::vector<StmtPtr> Optimizer::optimizeStmts(const std::vector<StmtPtr>& stmts) {
    std::vector<StmtPtr> result;
    for (const auto& s : stmts) {
        // one statement can turn into zero or many (like a folded if)
        for (const auto& out : optimizeStmt(s)) {
            result.push_back(out);
        }
    }
    return result;
}

std::vector<StmtPtr> Optimizer::optimizeStmt(const StmtPtr& stmt) {
    switch (stmt->kind) {
        case StmtKind::VarDeclaration: {
            ExprPtr value = optimizeExpr(stmt->expr);

            // remember the value if we know it, so later uses can be replaced
            auto c = evalConst(value);
            if (c) {
                constants[stmt->name] = *c;
            } else {
                constants.erase(stmt->name);
            }

            return {makeVarDecl(stmt->name, value)};
        }

        case StmtKind::Print:
            return {makePrint(optimizeExpr(stmt->expr))};

        case StmtKind::Block:
            return {makeBlock(optimizeStmts(stmt->body))};

        case StmtKind::If:
            return optimizeIf(stmt);

        case StmtKind::ExprStmt:
            return {makeExprStmt(optimizeExpr(stmt->expr))};

        case StmtKind::Paywall:
            return {makePaywall(stmt->amount)};
    }
    return {stmt};
}

std::vector<StmtPtr> Optimizer::optimizeIf(const StmtPtr& stmt) {
    ExprPtr cond = optimizeExpr(stmt->expr);

    // if we know the condition at compile time, throw away the dead branch
    auto c = evalConst(cond);
    if (c && c->kind == ConstValue::Kind::Bool) {
        if (c->boolValue) {
            return optimizeStmts(stmt->body);
        }
        if (stmt->hasElse) {
            return optimizeStmts(stmt->elseBody);
        }
        return {};
    }

    std::vector<StmtPtr> thenBody = optimizeStmts(stmt->body);
    std::vector<StmtPtr> elseBody;
    if (stmt->hasElse) {
        elseBody = optimizeStmts(stmt->elseBody);
    }

    return {makeIf(cond, thenBody, stmt->hasElse, elseBody)};
}

// -------- EXPRESSIONS --------

ExprPtr Optimizer::optimizeExpr(const ExprPtr& expr) {
    switch (expr->kind) {
        case ExprKind::Identifier: {
            // constant propagation: swap the name for its known value
            auto it = constants.find(expr->text);
            if (it == constants.end()) {
                return makeIdentifier(expr->text);
            }

            const ConstValue& c = it->second;
            if (c.kind == ConstValue::Kind::Int) return makeInt(c.intValue);
            if (c.kind == ConstValue::Kind::String) return makeString(c.stringValue);
            return makeBool(c.boolValue);
        }

        case ExprKind::Binary:
            return optimizeBinary(expr);

        case ExprKind::Assign: {
            ExprPtr v = optimizeExpr(expr->value);
            // if a variable is reassigned, its known constant value is invalid
            constants.erase(expr->text);
            return makeAssign(expr->text, v);
        }

        default:
            return expr;
    }
}

ExprPtr Optimizer::optimizeBinary(const ExprPtr& expr) {
    ExprPtr l = optimizeExpr(expr->left);
    ExprPtr r = optimizeExpr(expr->right);
    BinOp op = expr->op;

    // constant folding (e.g. 2 + 2 -> 4)
    auto lc = evalConst(l);
    auto rc = evalConst(r);
    if (lc && rc) {
        ExprPtr result = fold(*lc, op, *rc);
        if (result) {
            return result;
        }
    }

    // identity optimization (e.g. x + 0 -> x)
    bool lIsZero = l->kind == ExprKind::IntegerLiteral && l->intValue == 0;
    bool rIsZero = r->kind == ExprKind::IntegerLiteral && r->intValue == 0;

    if (op == BinOp::Add && lIsZero) return r;
    if (op == BinOp::Add && rIsZero) return l;
    if (op == BinOp::Sub && rIsZero) return l;

    return makeBinary(l, op, r);
}

// -------- CONSTANT FOLDING --------

std::optional<ConstValue> Optimizer::evalConst(const ExprPtr& expr) const {
    ConstValue c;
    switch (expr->kind) {
        case ExprKind::IntegerLiteral:
            c.kind = ConstValue::Kind::Int;
            c.intValue = expr->intValue;
            return c;
        case ExprKind::StringLiteral:
            c.kind = ConstValue::Kind::String;
            c.stringValue = expr->text;
            return c;
        case ExprKind::BooleanLiteral:
            c.kind = ConstValue::Kind::Bool;
            c.boolValue = expr->boolValue;
            return c;
        default:
            return std::nullopt;
    }
}

ExprPtr Optimizer::fold(const ConstValue& l, BinOp op, const ConstValue& r) const {
    using K = ConstValue::Kind;

    if (l.kind == K::Int && r.kind == K::Int) {
        // do the math in 64 bits then cut back to 32 so overflow wraps instead of being UB
        int64_t a = l.intValue;
        int64_t b = r.intValue;
        switch (op) {
            case BinOp::Add:         return makeInt(static_cast<int32_t>(a + b));
            case BinOp::Sub:         return makeInt(static_cast<int32_t>(a - b));
            case BinOp::GreaterThan: return makeBool(a > b);
            case BinOp::LessThan:    return makeBool(a < b);
        }
    }

    if (l.kind == K::String && r.kind == K::String && op == BinOp::Add) {
        return makeString(l.stringValue + r.stringValue);
    }

    return nullptr;
}

// -------- DEAD CODE ANALYSIS --------

void Optimizer::collectUsedVars(const std::vector<StmtPtr>& stmts) {
    for (const auto& s : stmts) {
        collectStmt(s);
    }
}

void Optimizer::collectStmt(const StmtPtr& stmt) {
    switch (stmt->kind) {
        case StmtKind::VarDeclaration:
        case StmtKind::Print:
        case StmtKind::ExprStmt:
            collectExpr(stmt->expr);
            break;

        case StmtKind::If:
            collectExpr(stmt->expr);
            collectUsedVars(stmt->body);
            collectUsedVars(stmt->elseBody);
            break;

        case StmtKind::Block:
            collectUsedVars(stmt->body);
            break;

        case StmtKind::Paywall:
            break;
    }
}

void Optimizer::collectExpr(const ExprPtr& expr) {
    switch (expr->kind) {
        case ExprKind::Identifier:
            usedVars.insert(expr->text);
            break;
        case ExprKind::Binary:
            collectExpr(expr->left);
            collectExpr(expr->right);
            break;
        case ExprKind::Assign:
            usedVars.insert(expr->text);
            collectExpr(expr->value);
            break;
        default:
            break;
    }
}

std::vector<StmtPtr> Optimizer::deadCodeElimination(const std::vector<StmtPtr>& stmts) const {
    std::vector<StmtPtr> result;
    for (const auto& s : stmts) {
        // if a variable is declared but never used, delete it
        if (s->kind == StmtKind::VarDeclaration && !usedVars.count(s->name)) {
            continue;
        }
        result.push_back(s);
    }
    return result;
}
