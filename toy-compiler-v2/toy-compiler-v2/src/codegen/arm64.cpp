#include "arm64.h"

std::string Codegen::generate(const std::vector<StmtPtr>& stmts) {
    // 1. DATA SECTION
    std::string header = "\t.data\n";
    header += "fmt_int: .asciz \"%d\\n\"\n";
    header += "fmt_str: .asciz \"%s\\n\"\n";

    // paywall strings
    header += "fmt_scan: .asciz \"%s\"\n";
    header += "msg_pay: .asciz \"free trial over pew pew, type 'haha' to continue: \"\n";
    header += "secret:  .asciz \"haha\"\n";

    // 2. TEXT SECTION
    header += "\n\t.text\n";
    header += "\t.global main\n";
    header += "main:\n";

    // prologue: save frame pointer + return address, make room for variables
    header += "\tstp x29, x30, [sp, #-16]!\n";
    header += "\tmov x29, sp\n";
    header += "\tsub sp, sp, #512\n";

    // generate statements (fills in this->out)
    for (const auto& s : stmts) {
        genStmt(s);
    }

    // epilogue
    emit("\tadd sp, sp, #512");
    emit("\tldp x29, x30, [sp], #16");
    emit("\tmov x0, #0");
    emit("\tret");

    return header + out;
}

// --- STATEMENT GENERATION ---

void Codegen::genStmt(const StmtPtr& stmt) {
    switch (stmt->kind) {
        case StmtKind::VarDeclaration: {
            std::string r = genExpr(stmt->expr);

            // give the variable a stack slot the first time we see it
            size_t offset;
            auto it = vars.find(stmt->name);
            if (it != vars.end()) {
                offset = it->second;
            } else {
                offset = stackOffset;
                vars[stmt->name] = offset;
                stackOffset += 8;
            }

            emit("\tstr " + r + ", [sp, #" + std::to_string(offset) + "]");
            break;
        }

        case StmtKind::Print: {
            std::string r = genExpr(stmt->expr);
            emit("\tadrp x0, fmt_int");
            emit("\tadd  x0, x0, :lo12:fmt_int");
            emit("\tmov x1, " + r);
            emit("\tbl printf");
            break;
        }

        case StmtKind::Block:
            for (const auto& s : stmt->body) {
                genStmt(s);
            }
            break;

        case StmtKind::If: {
            std::string condReg = genExpr(stmt->expr);
            std::string labelElse = label("else");
            std::string labelEnd = label("endif");

            // condition is 0 or 1, jump to else if it's 0
            emit("\tcmp " + condReg + ", #0");
            emit("\tbeq " + labelElse);

            for (const auto& s : stmt->body) {
                genStmt(s);
            }
            emit("\tb " + labelEnd);

            emit(labelElse + ":");
            for (const auto& s : stmt->elseBody) {
                genStmt(s);
            }
            emit(labelEnd + ":");
            break;
        }

        case StmtKind::ExprStmt:
            genExpr(stmt->expr);
            break;

        // --- PAYWALL ---
        case StmtKind::Paywall: {
            emit("\tadrp x0, msg_pay");
            emit("\tadd x0, x0, :lo12:msg_pay");
            emit("\tbl printf");

            emit("\tadrp x0, fmt_scan");
            emit("\tadd x0, x0, :lo12:fmt_scan");
            emit("\tadd x1, sp, #400"); // buffer at sp+400
            emit("\tbl scanf");

            emit("\tadd x0, sp, #400");
            emit("\tadrp x1, secret");
            emit("\tadd x1, x1, :lo12:secret");
            emit("\tbl strcmp");

            std::string labelPaid = label("paid");
            emit("\tcmp x0, #0");
            emit("\tbeq " + labelPaid);

            // exit if wrong (syscall 93 = exit)
            emit("\tmov x0, #1");
            emit("\tmov x8, #93");
            emit("\tsvc #0");

            emit(labelPaid + ":");
            break;
        }
    }
}

// --- EXPRESSION GENERATION ---

std::string Codegen::genExpr(const ExprPtr& expr) {
    switch (expr->kind) {
        case ExprKind::IntegerLiteral: {
            std::string r = allocTmp();
            emit("\tldr " + r + ", =" + std::to_string(expr->intValue));
            return r;
        }

        case ExprKind::Identifier: {
            std::string r = allocTmp();
            size_t offset = 0;
            auto it = vars.find(expr->text);
            if (it != vars.end()) {
                offset = it->second;
            }
            emit("\tldr " + r + ", [sp, #" + std::to_string(offset) + "]");
            return r;
        }

        case ExprKind::Binary: {
            std::string r1 = genExpr(expr->left);
            std::string r2 = genExpr(expr->right);
            std::string dest = allocTmp();

            switch (expr->op) {
                case BinOp::Add:
                    emit("\tadd " + dest + ", " + r1 + ", " + r2);
                    break;
                case BinOp::Sub:
                    emit("\tsub " + dest + ", " + r1 + ", " + r2);
                    break;
                case BinOp::GreaterThan:
                    emit("\tcmp " + r1 + ", " + r2);
                    emit("\tcset " + dest + ", gt");
                    break;
                case BinOp::LessThan:
                    emit("\tcmp " + r1 + ", " + r2);
                    emit("\tcset " + dest + ", lt");
                    break;
            }
            return dest;
        }

        case ExprKind::Maybe: {
            // coin flip: rand() and keep the lowest bit
            std::string r = allocTmp();
            emit("\tbl rand");
            emit("\tand " + r + ", x0, #1");
            return r;
        }

        default: {
            std::string r = allocTmp();
            emit("\tmov " + r + ", #0");
            return r;
        }
    }
}

// TODO: real register pool. right now everything goes through x14,
// so something like (a + b) with two variables overwrites a before the add
std::string Codegen::allocTmp() {
    return "x14";
}

std::string Codegen::label(const std::string& prefix) {
    std::string l = prefix + "_" + std::to_string(labelCounter);
    labelCounter++;
    return l;
}

void Codegen::emit(const std::string& line) {
    out += line;
    out += '\n';
}
