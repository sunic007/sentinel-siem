#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace sentinel::spl {

enum class TokenType {
    // Literals
    STRING,         // "hello" or 'hello'
    NUMBER,         // 42, 3.14
    IDENTIFIER,     // field names, index names

    // Keywords
    KW_SEARCH,
    KW_WHERE,
    KW_STATS,
    KW_TABLE,
    KW_FIELDS,
    KW_SORT,
    KW_HEAD,
    KW_TAIL,
    KW_EVAL,
    KW_TIMECHART,
    KW_DEDUP,
    KW_RENAME,
    KW_REX,
    KW_BY,
    KW_AS,
    KW_AND,
    KW_OR,
    KW_NOT,
    KW_IN,
    KW_LIKE,
    KW_INDEX,
    KW_SOURCETYPE,
    KW_HOST,
    KW_SOURCE,
    KW_SPAN,
    KW_TRUE,
    KW_FALSE,
    KW_NULL,

    // Aggregation functions
    KW_COUNT,
    KW_SUM,
    KW_AVG,
    KW_MIN,
    KW_MAX,
    KW_DC,
    KW_VALUES,
    KW_LIST,
    KW_FIRST,
    KW_LAST,

    // Operators
    PIPE,           // |
    EQUALS,         // =
    NOT_EQUALS,     // !=
    LESS_THAN,      // <
    GREATER_THAN,   // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL,  // >=
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    COMMA,          // ,
    DOT,            // .
    LPAREN,         // (
    RPAREN,         // )
    LBRACKET,       // [
    RBRACKET,       // ]

    // Special
    WILDCARD,       // *
    END_OF_INPUT,
    ERROR
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
};

class Lexer {
public:
    explicit Lexer(std::string_view input);

    // Tokenize the entire input
    std::vector<Token> tokenize();

    // Get next token
    Token next_token();

    // Peek at next token without consuming
    Token peek();

    bool has_more() const;

private:
    std::string_view input_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    void skip_whitespace();
    Token read_string();
    Token read_number();
    Token read_identifier_or_keyword();
    Token read_operator();

    char current() const;
    char advance();
    bool match(char expected);
    bool is_at_end() const;

    static TokenType keyword_type(std::string_view word);
};

} // namespace sentinel::spl
