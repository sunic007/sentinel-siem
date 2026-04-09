#pragma once

#include "spl/lexer.h"
#include "spl/ast.h"
#include <string>
#include <memory>
#include <vector>

namespace sentinel::spl {

struct ParseError {
    std::string message;
    size_t line;
    size_t column;
};

class Parser {
public:
    explicit Parser(std::string_view input);

    // Parse the complete SPL query
    std::unique_ptr<Query> parse();

    // Get parse errors
    const std::vector<ParseError>& errors() const { return errors_; }

    bool has_errors() const { return !errors_.empty(); }

private:
    Lexer lexer_;
    std::vector<Token> tokens_;
    size_t pos_ = 0;
    std::vector<ParseError> errors_;

    // Command parsers
    std::unique_ptr<CommandNode> parse_command();
    std::unique_ptr<SearchNode> parse_search();
    std::unique_ptr<WhereNode> parse_where();
    std::unique_ptr<StatsNode> parse_stats();
    std::unique_ptr<TableNode> parse_table();
    std::unique_ptr<FieldsNode> parse_fields();
    std::unique_ptr<SortNode> parse_sort();
    std::unique_ptr<HeadNode> parse_head();
    std::unique_ptr<TailNode> parse_tail();
    std::unique_ptr<EvalNode> parse_eval();
    std::unique_ptr<TimechartNode> parse_timechart();
    std::unique_ptr<DedupNode> parse_dedup();
    std::unique_ptr<RenameNode> parse_rename();

    // Expression parsers
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_or_expression();
    std::unique_ptr<Expression> parse_and_expression();
    std::unique_ptr<Expression> parse_comparison();
    std::unique_ptr<Expression> parse_primary();

    // Aggregation parser
    AggSpec parse_agg_spec();

    // Token helpers
    const Token& current() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& message);
    bool is_at_end() const;

    void error(const std::string& message);
    void synchronize();

    // Utility
    std::vector<std::string> parse_field_list();
};

} // namespace sentinel::spl
