#pragma once

#include <cstdint>
#include <string>

enum class TokenType {
    // keywords
    Print,
    If,
    Else,
    Int,
    Maybe,
    Paywall,

    // identifiers & literals
    Identifier,
    IntegerLiteral,
    StringLiteral,

    // operators
    Plus,
    Minus,
    Assign,
    GreaterThan,
    LessThan,

    // punctuation
    SemiColon,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
};

struct Token {
    TokenType type;
    std::string text;   // the name for Identifier, the contents for StringLiteral
    int64_t number = 0; // only used by IntegerLiteral
};

// the regex pattern that matches each kind of token
std::string tokenRegex(TokenType type);
