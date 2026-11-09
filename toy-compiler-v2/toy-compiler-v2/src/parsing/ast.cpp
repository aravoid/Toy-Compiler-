#include "ast.h"

ExprPtr makeInt(int32_t n) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::IntegerLiteral;
    e->intValue = n;
    return e;
}

ExprPtr makeString(const std::string& s) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::StringLiteral;
    e->text = s;
    return e;
}

ExprPtr makeBool(bool b) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::BooleanLiteral;
    e->boolValue = b;
    return e;
}

ExprPtr makeIdentifier(const std::string& name) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::Identifier;
    e->text = name;
    return e;
}

ExprPtr makeMaybe() {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::Maybe;
    return e;
}

ExprPtr makeBinary(ExprPtr left, BinOp op, ExprPtr right) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::Binary;
    e->left = left;
    e->op = op;
    e->right = right;
    return e;
}

ExprPtr makeAssign(const std::string& name, ExprPtr value) {
    auto e = std::make_shared<Expr>();
    e->kind = ExprKind::Assign;
    e->text = name;
    e->value = value;
    return e;
}

StmtPtr makePrint(ExprPtr e) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Print;
    s->expr = e;
    return s;
}

StmtPtr makeVarDecl(const std::string& name, ExprPtr value) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::VarDeclaration;
    s->name = name;
    s->expr = value;
    return s;
}

StmtPtr makeIf(ExprPtr cond, std::vector<StmtPtr> thenBody, bool hasElse, std::vector<StmtPtr> elseBody) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::If;
    s->expr = cond;
    s->body = thenBody;
    s->hasElse = hasElse;
    s->elseBody = elseBody;
    return s;
}

StmtPtr makeBlock(std::vector<StmtPtr> stmts) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Block;
    s->body = stmts;
    return s;
}

StmtPtr makeExprStmt(ExprPtr e) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::ExprStmt;
    s->expr = e;
    return s;
}

StmtPtr makePaywall(int64_t amount) {
    auto s = std::make_shared<Stmt>();
    s->kind = StmtKind::Paywall;
    s->amount = amount;
    return s;
}

// ---------- comparison ----------

bool sameExpr(const ExprPtr& a, const ExprPtr& b) {
    if (a == b) return true; // same node (or both null)
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
        case ExprKind::IntegerLiteral: return a->intValue == b->intValue;
        case ExprKind::StringLiteral:  return a->text == b->text;
        case ExprKind::BooleanLiteral: return a->boolValue == b->boolValue;
        case ExprKind::Identifier:     return a->text == b->text;
        case ExprKind::Maybe:          return true;
        case ExprKind::Binary:
            return a->op == b->op && sameExpr(a->left, b->left) && sameExpr(a->right, b->right);
        case ExprKind::Assign:
            return a->text == b->text && sameExpr(a->value, b->value);
    }
    return false;
}

bool sameStmt(const StmtPtr& a, const StmtPtr& b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
        case StmtKind::Print:
        case StmtKind::ExprStmt:
            return sameExpr(a->expr, b->expr);
        case StmtKind::VarDeclaration:
            return a->name == b->name && sameExpr(a->expr, b->expr);
        case StmtKind::If:
            return sameExpr(a->expr, b->expr) && sameStmts(a->body, b->body) &&
                   a->hasElse == b->hasElse && sameStmts(a->elseBody, b->elseBody);
        case StmtKind::Block:
            return sameStmts(a->body, b->body);
        case StmtKind::Paywall:
            return a->amount == b->amount;
    }
    return false;
}

bool sameStmts(const std::vector<StmtPtr>& a, const std::vector<StmtPtr>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (!sameStmt(a[i], b[i])) return false;
    }
    return true;
}
