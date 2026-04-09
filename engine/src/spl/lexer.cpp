#include "spl/lexer.h"
#include <cctype>
#include <algorithm>
#include <unordered_map>

namespace sentinel::spl {

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"search", TokenType::KW_SEARCH},
    {"where", TokenType::KW_WHERE},
    {"stats", TokenType::KW_STATS},
    {"table", TokenType::KW_TABLE},
    {"fields", TokenType::KW_FIELDS},
    {"sort", TokenType::KW_SORT},
    {"head", TokenType::KW_HEAD},
    {"tail", TokenType::KW_TAIL},
    {"eval", TokenType::KW_EVAL},
    {"timechart", TokenType::KW_TIMECHART},
    {"dedup", TokenType::KW_DEDUP},
    {"rename", TokenType::KW_RENAME},
    {"rex", TokenType::KW_REX},
    {"by", TokenType::KW_BY},
    {"as", TokenType::KW_AS},
    {"and", TokenType::KW_AND},
    {"or", TokenType::KW_OR},
    {"not", TokenType::KW_NOT},
    {"in", TokenType::KW_IN},
    {"like", TokenType::KW_LIKE},
    {"index", TokenType::KW_INDEX},
    {"sourcetype", TokenType::KW_SOURCETYPE},
    {"host", TokenType::KW_HOST},
    {"source", TokenType::KW_SOURCE},
    {"span", TokenType::KW_SPAN},
    {"true", TokenType::KW_TRUE},
    {"false", TokenType::KW_FALSE},
    {"null", TokenType::KW_NULL},
    {"count", TokenType::KW_COUNT},
    {"sum", TokenType::KW_SUM},
    {"avg", TokenType::KW_AVG},
    {"min", TokenType::KW_MIN},
    {"max", TokenType::KW_MAX},
    {"dc", TokenType::KW_DC},
    {"values", TokenType::KW_VALUES},
    {"list", TokenType::KW_LIST},
    {"first", TokenType::KW_FIRST},
    {"last", TokenType::KW_LAST},
};

Lexer::Lexer(std::string_view input) : input_(input) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (has_more()) {
        auto token = next_token();
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_INPUT ||
            token.type == TokenType::ERROR) {
            break;
        }
    }
    return tokens;
}

Token Lexer::next_token() {
    skip_whitespace();

    if (is_at_end()) {
        return {TokenType::END_OF_INPUT, "", line_, column_};
    }

    char c = current();

    // Strings
    if (c == '"' || c == '\'') {
        return read_string();
    }

    // Numbers
    if (std::isdigit(c) || (c == '-' && pos_ + 1 < input_.size() && std::isdigit(input_[pos_ + 1]))) {
        return read_number();
    }

    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') {
        return read_identifier_or_keyword();
    }

    // Operators
    return read_operator();
}

Token Lexer::peek() {
    size_t saved_pos = pos_;
    size_t saved_line = line_;
    size_t saved_col = column_;

    auto token = next_token();

    pos_ = saved_pos;
    line_ = saved_line;
    column_ = saved_col;

    return token;
}

bool Lexer::has_more() const {
    return pos_ < input_.size();
}

void Lexer::skip_whitespace() {
    while (!is_at_end() && std::isspace(current())) {
        if (current() == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
        ++pos_;
    }
}

Token Lexer::read_string() {
    char quote = advance();
    size_t start_line = line_;
    size_t start_col = column_ - 1;
    std::string value;

    while (!is_at_end() && current() != quote) {
        if (current() == '\\' && pos_ + 1 < input_.size()) {
            advance();
            switch (current()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '\\': value += '\\'; break;
                default: value += current(); break;
            }
        } else {
            value += current();
        }
        advance();
    }

    if (!is_at_end()) advance();  // Consume closing quote

    return {TokenType::STRING, value, start_line, start_col};
}

Token Lexer::read_number() {
    size_t start = pos_;
    size_t start_col = column_;

    if (current() == '-') advance();

    while (!is_at_end() && std::isdigit(current())) advance();

    if (!is_at_end() && current() == '.') {
        advance();
        while (!is_at_end() && std::isdigit(current())) advance();
    }

    return {TokenType::NUMBER,
            std::string(input_.substr(start, pos_ - start)),
            line_, start_col};
}

Token Lexer::read_identifier_or_keyword() {
    size_t start = pos_;
    size_t start_col = column_;

    while (!is_at_end() && (std::isalnum(current()) || current() == '_' || current() == '.')) {
        advance();
    }

    std::string word(input_.substr(start, pos_ - start));
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto type = keyword_type(lower);
    return {type, word, line_, start_col};
}

Token Lexer::read_operator() {
    size_t start_col = column_;
    char c = advance();

    switch (c) {
        case '|': return {TokenType::PIPE, "|", line_, start_col};
        case ',': return {TokenType::COMMA, ",", line_, start_col};
        case '(': return {TokenType::LPAREN, "(", line_, start_col};
        case ')': return {TokenType::RPAREN, ")", line_, start_col};
        case '[': return {TokenType::LBRACKET, "[", line_, start_col};
        case ']': return {TokenType::RBRACKET, "]", line_, start_col};
        case '+': return {TokenType::PLUS, "+", line_, start_col};
        case '-': return {TokenType::MINUS, "-", line_, start_col};
        case '*': return {TokenType::STAR, "*", line_, start_col};
        case '/': return {TokenType::SLASH, "/", line_, start_col};
        case '.': return {TokenType::DOT, ".", line_, start_col};
        case '=': return {TokenType::EQUALS, "=", line_, start_col};
        case '!':
            if (match('=')) return {TokenType::NOT_EQUALS, "!=", line_, start_col};
            return {TokenType::ERROR, "!", line_, start_col};
        case '<':
            if (match('=')) return {TokenType::LESS_EQUAL, "<=", line_, start_col};
            return {TokenType::LESS_THAN, "<", line_, start_col};
        case '>':
            if (match('=')) return {TokenType::GREATER_EQUAL, ">=", line_, start_col};
            return {TokenType::GREATER_THAN, ">", line_, start_col};
        default:
            return {TokenType::ERROR, std::string(1, c), line_, start_col};
    }
}

char Lexer::current() const {
    return input_[pos_];
}

char Lexer::advance() {
    char c = input_[pos_++];
    ++column_;
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end() || input_[pos_] != expected) return false;
    ++pos_;
    ++column_;
    return true;
}

bool Lexer::is_at_end() const {
    return pos_ >= input_.size();
}

TokenType Lexer::keyword_type(std::string_view word) {
    auto it = KEYWORDS.find(std::string(word));
    if (it != KEYWORDS.end()) return it->second;
    return TokenType::IDENTIFIER;
}

} // namespace sentinel::spl
