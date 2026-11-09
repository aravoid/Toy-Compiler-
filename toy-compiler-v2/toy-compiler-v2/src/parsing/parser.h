#pragma once

#include <stdexcept>
#include <vector>

#include "../lexing/token.h"
#include "ast.h"

// thrown whenever the parser sees something it didn't expect
struct ParseError : std::runtime_error {
    ParseError() : std::runtime_error("UnexpectedToken") {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // the whole program comes back wrapped in one Block
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    // utilities
    const Token* current() const; // nullptr at the end
    bool check(TokenType type) const;
    void advance();
    void expect(TokenType type);

    // statements
    StmtPtr parseStmt();
    StmtPtr parseBlockStmt();
    StmtPtr parsePrint();
    StmtPtr parseVarDecl();
    StmtPtr parseIf();
    StmtPtr parsePaywall();

    // expressions (lowest precedence first)
    ExprPtr parseExpr();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parsePrimary();
};
