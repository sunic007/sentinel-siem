#include "spl/parser.h"
#include <spdlog/spdlog.h>

namespace sentinel::spl {

Parser::Parser(std::string_view input) : lexer_(input) {
    tokens_ = lexer_.tokenize();
}

std::unique_ptr<Query> Parser::parse() {
    auto query = std::make_unique<Query>();

    // First command is implicitly 'search' if not specified
    if (!is_at_end()) {
        auto first_cmd = parse_command();
        if (first_cmd) {
            query->commands.push_back(std::move(first_cmd));
        }
    }

    // Parse pipe-separated commands
    while (match(TokenType::PIPE)) {
        auto cmd = parse_command();
        if (cmd) {
            query->commands.push_back(std::move(cmd));
        }
    }

    return query;
}

std::unique_ptr<CommandNode> Parser::parse_command() {
    if (is_at_end()) {
        error("Expected command");
        return nullptr;
    }

    const auto& tok = current();

    switch (tok.type) {
        case TokenType::KW_SEARCH:  advance(); return parse_search();
        case TokenType::KW_WHERE:   advance(); return parse_where();
        case TokenType::KW_STATS:   advance(); return parse_stats();
        case TokenType::KW_TABLE:   advance(); return parse_table();
        case TokenType::KW_FIELDS:  advance(); return parse_fields();
        case TokenType::KW_SORT:    advance(); return parse_sort();
        case TokenType::KW_HEAD:    advance(); return parse_head();
        case TokenType::KW_TAIL:    advance(); return parse_tail();
        case TokenType::KW_EVAL:    advance(); return parse_eval();
        case TokenType::KW_TIMECHART: advance(); return parse_timechart();
        case TokenType::KW_DEDUP:   advance(); return parse_dedup();
        case TokenType::KW_RENAME:  advance(); return parse_rename();
        default:
            // Implicit search command
            return parse_search();
    }
}

std::unique_ptr<SearchNode> Parser::parse_search() {
    auto node = std::make_unique<SearchNode>();
    node->type = CommandType::SEARCH;

    while (!is_at_end() && !check(TokenType::PIPE)) {
        if (check(TokenType::KW_INDEX) || (check(TokenType::IDENTIFIER) && current().value == "index")) {
            advance();
            expect(TokenType::EQUALS, "Expected '=' after 'index'");
            node->index = current().value;
            advance();
        } else if (check(TokenType::KW_SOURCETYPE)) {
            advance();
            expect(TokenType::EQUALS, "Expected '=' after 'sourcetype'");
            node->sourcetype = current().value;
            advance();
        } else if (check(TokenType::KW_HOST)) {
            advance();
            expect(TokenType::EQUALS, "Expected '=' after 'host'");
            node->host = current().value;
            advance();
        } else if (check(TokenType::KW_SOURCE)) {
            advance();
            expect(TokenType::EQUALS, "Expected '=' after 'source'");
            node->source = current().value;
            advance();
        } else {
            // Search term
            node->search_terms.push_back(current().value);
            advance();
        }
    }

    return node;
}

std::unique_ptr<WhereNode> Parser::parse_where() {
    auto node = std::make_unique<WhereNode>();
    node->type = CommandType::WHERE;
    node->condition = parse_expression();
    return node;
}

std::unique_ptr<StatsNode> Parser::parse_stats() {
    auto node = std::make_unique<StatsNode>();
    node->type = CommandType::STATS;

    // Parse aggregations
    node->aggregations.push_back(parse_agg_spec());
    while (match(TokenType::COMMA)) {
        node->aggregations.push_back(parse_agg_spec());
    }

    // Parse group-by
    if (match(TokenType::KW_BY)) {
        node->group_by = parse_field_list();
    }

    return node;
}

std::unique_ptr<TableNode> Parser::parse_table() {
    auto node = std::make_unique<TableNode>();
    node->type = CommandType::TABLE;
    node->fields = parse_field_list();
    return node;
}

std::unique_ptr<FieldsNode> Parser::parse_fields() {
    auto node = std::make_unique<FieldsNode>();
    node->type = CommandType::FIELDS;

    if (match(TokenType::MINUS)) {
        node->include = false;
    } else if (match(TokenType::PLUS)) {
        node->include = true;
    }

    node->fields = parse_field_list();
    return node;
}

std::unique_ptr<SortNode> Parser::parse_sort() {
    auto node = std::make_unique<SortNode>();
    node->type = CommandType::SORT;

    while (!is_at_end() && !check(TokenType::PIPE)) {
        SortDir dir = SortDir::ASC;
        if (match(TokenType::MINUS)) {
            dir = SortDir::DESC;
        } else if (match(TokenType::PLUS)) {
            dir = SortDir::ASC;
        }

        std::string field = current().value;
        advance();
        node->fields.push_back({field, dir});

        if (!match(TokenType::COMMA)) break;
    }

    return node;
}

std::unique_ptr<HeadNode> Parser::parse_head() {
    auto node = std::make_unique<HeadNode>();
    node->type = CommandType::HEAD;

    if (check(TokenType::NUMBER)) {
        node->count = std::stoi(current().value);
        advance();
    }

    return node;
}

std::unique_ptr<TailNode> Parser::parse_tail() {
    auto node = std::make_unique<TailNode>();
    node->type = CommandType::TAIL;

    if (check(TokenType::NUMBER)) {
        node->count = std::stoi(current().value);
        advance();
    }

    return node;
}

std::unique_ptr<EvalNode> Parser::parse_eval() {
    auto node = std::make_unique<EvalNode>();
    node->type = CommandType::EVAL;

    node->output_field = current().value;
    advance();
    expect(TokenType::EQUALS, "Expected '=' in eval");
    node->expression = parse_expression();

    return node;
}

std::unique_ptr<TimechartNode> Parser::parse_timechart() {
    auto node = std::make_unique<TimechartNode>();
    node->type = CommandType::TIMECHART;

    // Parse optional span
    if (check(TokenType::KW_SPAN)) {
        advance();
        expect(TokenType::EQUALS, "Expected '=' after 'span'");
        node->span = current().value;
        advance();
    }

    // Parse aggregation
    node->aggregations.push_back(parse_agg_spec());

    // Parse optional by clause
    if (match(TokenType::KW_BY)) {
        node->split_by = current().value;
        advance();
    }

    return node;
}

std::unique_ptr<DedupNode> Parser::parse_dedup() {
    auto node = std::make_unique<DedupNode>();
    node->type = CommandType::DEDUP;

    // Optional count
    if (check(TokenType::NUMBER)) {
        node->keep_count = std::stoi(current().value);
        advance();
    }

    node->fields = parse_field_list();
    return node;
}

std::unique_ptr<RenameNode> Parser::parse_rename() {
    auto node = std::make_unique<RenameNode>();
    node->type = CommandType::RENAME;

    while (!is_at_end() && !check(TokenType::PIPE)) {
        std::string old_name = current().value;
        advance();
        expect(TokenType::KW_AS, "Expected 'AS' in rename");
        std::string new_name = current().value;
        advance();
        node->mappings.push_back({old_name, new_name});

        if (!match(TokenType::COMMA)) break;
    }

    return node;
}

// Expression parsing (recursive descent)
std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_or_expression();
}

std::unique_ptr<Expression> Parser::parse_or_expression() {
    auto left = parse_and_expression();

    while (match(TokenType::KW_OR)) {
        auto right = parse_and_expression();
        auto expr = std::make_unique<LogicExpr>();
        expr->op = LogicOp::OR;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_and_expression() {
    auto left = parse_comparison();

    while (match(TokenType::KW_AND)) {
        auto right = parse_comparison();
        auto expr = std::make_unique<LogicExpr>();
        expr->op = LogicOp::AND;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_comparison() {
    if (match(TokenType::KW_NOT)) {
        auto expr = std::make_unique<LogicExpr>();
        expr->op = LogicOp::NOT;
        expr->left = parse_comparison();
        return expr;
    }

    auto left = parse_primary();

    if (check(TokenType::EQUALS) || check(TokenType::NOT_EQUALS) ||
        check(TokenType::LESS_THAN) || check(TokenType::GREATER_THAN) ||
        check(TokenType::LESS_EQUAL) || check(TokenType::GREATER_EQUAL)) {

        CompareOp op;
        switch (current().type) {
            case TokenType::EQUALS:        op = CompareOp::EQ; break;
            case TokenType::NOT_EQUALS:    op = CompareOp::NEQ; break;
            case TokenType::LESS_THAN:     op = CompareOp::LT; break;
            case TokenType::GREATER_THAN:  op = CompareOp::GT; break;
            case TokenType::LESS_EQUAL:    op = CompareOp::LTE; break;
            case TokenType::GREATER_EQUAL: op = CompareOp::GTE; break;
            default: op = CompareOp::EQ; break;
        }
        advance();

        auto right = parse_primary();
        auto expr = std::make_unique<CompareExpr>();
        expr->left = std::move(left);
        expr->op = op;
        expr->right = std::move(right);
        return expr;
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_primary() {
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    if (check(TokenType::STRING)) {
        auto expr = std::make_unique<LiteralExpr>();
        expr->value = current().value;
        advance();
        return expr;
    }

    if (check(TokenType::NUMBER)) {
        auto expr = std::make_unique<LiteralExpr>();
        expr->value = current().value;
        advance();
        return expr;
    }

    // Identifier (could be field name or function call)
    auto name = current().value;
    advance();

    if (match(TokenType::LPAREN)) {
        // Function call
        auto func = std::make_unique<FunctionExpr>();
        func->function_name = name;

        if (!check(TokenType::RPAREN)) {
            func->args.push_back(parse_expression());
            while (match(TokenType::COMMA)) {
                func->args.push_back(parse_expression());
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after function arguments");
        return func;
    }

    auto expr = std::make_unique<FieldExpr>();
    expr->field_name = name;
    return expr;
}

AggSpec Parser::parse_agg_spec() {
    AggSpec spec;

    auto tok = current();
    AggFunc func;
    bool is_agg = true;

    switch (tok.type) {
        case TokenType::KW_COUNT:  func = AggFunc::COUNT; break;
        case TokenType::KW_SUM:    func = AggFunc::SUM; break;
        case TokenType::KW_AVG:    func = AggFunc::AVG; break;
        case TokenType::KW_MIN:    func = AggFunc::MIN; break;
        case TokenType::KW_MAX:    func = AggFunc::MAX; break;
        case TokenType::KW_DC:     func = AggFunc::DC; break;
        case TokenType::KW_VALUES: func = AggFunc::VALUES; break;
        case TokenType::KW_LIST:   func = AggFunc::LIST; break;
        case TokenType::KW_FIRST:  func = AggFunc::FIRST; break;
        case TokenType::KW_LAST:   func = AggFunc::LAST; break;
        default:
            func = AggFunc::COUNT;
            is_agg = false;
            break;
    }

    if (is_agg) {
        advance();
        spec.func = func;

        if (match(TokenType::LPAREN)) {
            if (!check(TokenType::RPAREN)) {
                spec.field = current().value;
                advance();
            }
            expect(TokenType::RPAREN, "Expected ')' after aggregation function");
        }

        // Optional alias: count() AS total
        if (match(TokenType::KW_AS)) {
            spec.alias = current().value;
            advance();
        } else {
            // Default alias
            spec.alias = spec.field.empty()
                ? "count"
                : tok.value + "(" + spec.field + ")";
        }
    }

    return spec;
}

std::vector<std::string> Parser::parse_field_list() {
    std::vector<std::string> fields;

    if (!is_at_end() && !check(TokenType::PIPE)) {
        fields.push_back(current().value);
        advance();

        while (match(TokenType::COMMA)) {
            if (!is_at_end()) {
                fields.push_back(current().value);
                advance();
            }
        }
    }

    return fields;
}

// Token helpers
const Token& Parser::current() const {
    static Token eof{TokenType::END_OF_INPUT, "", 0, 0};
    if (pos_ >= tokens_.size()) return eof;
    return tokens_[pos_];
}

const Token& Parser::advance() {
    const auto& tok = current();
    if (pos_ < tokens_.size()) ++pos_;
    return tok;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(message + " (got '" + current().value + "')");
    return current();
}

bool Parser::is_at_end() const {
    return pos_ >= tokens_.size() || current().type == TokenType::END_OF_INPUT;
}

void Parser::error(const std::string& message) {
    errors_.push_back({message, current().line, current().column});
}

void Parser::synchronize() {
    while (!is_at_end()) {
        if (current().type == TokenType::PIPE) return;
        advance();
    }
}

} // namespace sentinel::spl
