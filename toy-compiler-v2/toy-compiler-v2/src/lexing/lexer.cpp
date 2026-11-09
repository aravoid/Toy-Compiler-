#include "lexer.h"

#include <algorithm>
#include <regex>

// one regex hit: which token it was and where it sits in the source
struct Match {
    TokenType type;
    size_t start;
    size_t end;
};

std::vector<Token> lexProgram(const std::string& program) {
    // when two matches start at the same spot with the same length,
    // the one earlier in this list wins. so keywords have to come
    // before Identifier, otherwise "print" would lex as a name.
    const TokenType order[] = {
        // keywords
        TokenType::Print,
        TokenType::If,
        TokenType::Else,
        TokenType::Int,
        TokenType::Maybe,
        TokenType::Paywall,

        // literals
        TokenType::IntegerLiteral,
        TokenType::StringLiteral,

        // operators
        TokenType::Plus,
        TokenType::Minus,
        TokenType::Assign,
        TokenType::GreaterThan,
        TokenType::LessThan,

        // punctuation
        TokenType::LeftParen,
        TokenType::RightParen,
        TokenType::LeftBrace,
        TokenType::RightBrace,
        TokenType::SemiColon,

        // identifiers (keep LAST)
        TokenType::Identifier,
    };

    // run every regex over the whole program and collect all the hits
    std::vector<Match> matches;
    for (TokenType type : order) {
        std::regex re(tokenRegex(type));
        auto begin = std::sregex_iterator(program.begin(), program.end(), re);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            size_t start = it->position();
            matches.push_back({type, start, start + it->length()});
        }
    }

    // sort by position, then longest match first
    // (stable so the list order above breaks ties)
    std::stable_sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        if (a.start != b.start) {
            return a.start < b.start;
        }
        return (a.end - a.start) > (b.end - b.start);
    });

    std::vector<Token> result;
    size_t lastEnd = 0;

    for (const Match& m : matches) {
        // skip anything overlapping a token we already took
        if (m.start < lastEnd) {
            continue;
        }
        lastEnd = m.end;

        std::string lexeme = program.substr(m.start, m.end - m.start);

        Token tok;
        tok.type = m.type;

        if (m.type == TokenType::IntegerLiteral) {
            tok.number = std::stoll(lexeme);
        } else if (m.type == TokenType::StringLiteral) {
            // remove surrounding quotes
            tok.text = lexeme.substr(1, lexeme.size() - 2);
        } else if (m.type == TokenType::Identifier) {
            tok.text = lexeme;
        }

        result.push_back(tok);
    }

    return result;
}
