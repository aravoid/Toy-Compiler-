#include "token.h"

#include <stdexcept>

std::string tokenRegex(TokenType type) {
    switch (type) {
        // keywords
        case TokenType::Print:   return R"(\bprint\b)";
        case TokenType::If:      return R"(\bif\b)";
        case TokenType::Else:    return R"(\belse\b)";
        case TokenType::Int:     return R"(\bint\b)";
        case TokenType::Maybe:   return R"(\bmaybe\b)";
        case TokenType::Paywall: return R"(\bpaywall\b)";

        // literals
        case TokenType::IntegerLiteral: return R"(\d+)";
        case TokenType::StringLiteral:  return R"("[^"]*")";

        // identifiers
        case TokenType::Identifier: return R"([a-zA-Z_][a-zA-Z0-9_]*)";

        // operators
        case TokenType::Plus:   return R"(\+)";
        case TokenType::Minus:  return R"(-)";
        case TokenType::Assign: return R"(=)";

        // punctuation
        case TokenType::SemiColon:  return R"(;)";
        case TokenType::LeftParen:  return R"(\()";
        case TokenType::RightParen: return R"(\))";
        case TokenType::LeftBrace:  return R"(\{)";
        case TokenType::RightBrace: return R"(\})";

        // logical operators
        case TokenType::GreaterThan: return R"(>)";
        case TokenType::LessThan:    return R"(<)";
    }
    throw std::runtime_error("invalid token type");
}
