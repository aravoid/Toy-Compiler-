#include "parser.h"

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

// ----------------- utilities -----------------

const Token* Parser::current() const {
    if (pos < tokens.size()) {
        return &tokens[pos];
    }
    return nullptr;
}

bool Parser::check(TokenType type) const {
    return current() != nullptr && current()->type == type;
}

void Parser::advance() {
    pos++;
}

void Parser::expect(TokenType type) {
    if (!check(type)) {
        throw ParseError();
    }
    advance();
}

// ----------------- entry -----------------

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;

    while (pos < tokens.size()) {
        stmts.push_back(parseStmt());
    }

    return {makeBlock(stmts)};
}

// ----------------- statements -----------------

StmtPtr Parser::parseStmt() {
    if (check(TokenType::Print))     return parsePrint();
    if (check(TokenType::If))        return parseIf();
    if (check(TokenType::Int))       return parseVarDecl();
    if (check(TokenType::LeftBrace)) return parseBlockStmt();
    if (check(TokenType::Paywall))   return parsePaywall();
    throw ParseError();
}

StmtPtr Parser::parseBlockStmt() {
    expect(TokenType::LeftBrace);

    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RightBrace)) {
        stmts.push_back(parseStmt());
    }

    expect(TokenType::RightBrace);
    return makeBlock(stmts);
}

StmtPtr Parser::parsePrint() {
    advance(); // consume 'print'
    expect(TokenType::LeftParen);
    ExprPtr e = parseExpr();
    expect(TokenType::RightParen);
    expect(TokenType::SemiColon);
    return makePrint(e);
}

StmtPtr Parser::parseVarDecl() {
    advance(); // consume 'int'

    if (!check(TokenType::Identifier)) {
        throw ParseError();
    }
    std::string name = current()->text;
    advance();

    expect(TokenType::Assign);
    ExprPtr value = parseExpr();
    expect(TokenType::SemiColon);

    return makeVarDecl(name, value);
}

StmtPtr Parser::parseIf() {
    advance(); // consume 'if'
    expect(TokenType::LeftParen);
    ExprPtr condition = parseExpr();
    expect(TokenType::RightParen);

    // both branches are parsed as blocks, then we just keep the statements inside
    StmtPtr thenBlock = parseBlockStmt();

    bool hasElse = false;
    std::vector<StmtPtr> elseBody;
    if (check(TokenType::Else)) {
        advance();
        hasElse = true;
        elseBody = parseBlockStmt()->body;
    }

    return makeIf(condition, thenBlock->body, hasElse, elseBody);
}

StmtPtr Parser::parsePaywall() {
    advance(); // consume 'paywall'
    expect(TokenType::LeftParen);

    // we expect a simple integer literal inside
    if (!check(TokenType::IntegerLiteral)) {
        throw ParseError();
    }
    int64_t amount = current()->number;
    advance(); // consume the number

    expect(TokenType::RightParen);
    expect(TokenType::SemiColon);

    return makePaywall(amount);
}

// ----------------- expressions -----------------

ExprPtr Parser::parseExpr() {
    return parseComparison();
}

ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddition();

    while (check(TokenType::GreaterThan) || check(TokenType::LessThan)) {
        BinOp op = check(TokenType::GreaterThan) ? BinOp::GreaterThan : BinOp::LessThan;
        advance();
        ExprPtr right = parseAddition();
        left = makeBinary(left, op, right);
    }

    return left;
}

ExprPtr Parser::parseAddition() {
    ExprPtr left = parsePrimary();

    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        BinOp op = check(TokenType::Plus) ? BinOp::Add : BinOp::Sub;
        advance();
        ExprPtr right = parsePrimary();
        left = makeBinary(left, op, right);
    }

    return left;
}

ExprPtr Parser::parsePrimary() {
    const Token* tok = current();
    if (tok == nullptr) {
        throw ParseError();
    }

    switch (tok->type) {
        case TokenType::IntegerLiteral: {
            int32_t v = static_cast<int32_t>(tok->number); // i64 -> i32
            advance();
            return makeInt(v);
        }
        case TokenType::StringLiteral: {
            std::string v = tok->text;
            advance();
            return makeString(v);
        }
        case TokenType::Identifier: {
            std::string v = tok->text;
            advance();
            return makeIdentifier(v);
        }
        case TokenType::Maybe:
            advance();
            return makeMaybe();
        case TokenType::LeftParen: {
            advance();
            ExprPtr e = parseExpr();
            expect(TokenType::RightParen);
            return e;
        }
        default:
            throw ParseError();
    }
}
