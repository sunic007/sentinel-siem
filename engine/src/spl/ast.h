#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace sentinel::spl {

// Forward declarations
struct SearchNode;
struct WhereNode;
struct StatsNode;
struct TableNode;
struct FieldsNode;
struct SortNode;
struct HeadNode;
struct TailNode;
struct EvalNode;
struct TimechartNode;
struct DedupNode;
struct RenameNode;
struct RexNode;

// SPL command types
enum class CommandType {
    SEARCH,
    WHERE,
    STATS,
    TABLE,
    FIELDS,
    SORT,
    HEAD,
    TAIL,
    EVAL,
    TIMECHART,
    DEDUP,
    RENAME,
    REX
};

// Comparison operators
enum class CompareOp {
    EQ,     // =
    NEQ,    // !=
    LT,     // <
    GT,     // >
    LTE,    // <=
    GTE,    // >=
    LIKE,   // LIKE
    IN      // IN
};

// Logical operators
enum class LogicOp {
    AND,
    OR,
    NOT
};

// Aggregation functions
enum class AggFunc {
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,
    DC,       // distinct count
    VALUES,
    LIST,
    FIRST,
    LAST
};

// Sort direction
enum class SortDir {
    ASC,
    DESC
};

// Expression node (for WHERE, EVAL conditions)
struct Expression {
    virtual ~Expression() = default;
};

struct LiteralExpr : Expression {
    std::string value;
};

struct FieldExpr : Expression {
    std::string field_name;
};

struct CompareExpr : Expression {
    std::unique_ptr<Expression> left;
    CompareOp op;
    std::unique_ptr<Expression> right;
};

struct LogicExpr : Expression {
    LogicOp op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;  // null for NOT
};

struct FunctionExpr : Expression {
    std::string function_name;
    std::vector<std::unique_ptr<Expression>> args;
};

// Aggregation specification
struct AggSpec {
    AggFunc func;
    std::string field;       // Field to aggregate (empty for count())
    std::string alias;       // Output field name
};

// Base command node
struct CommandNode {
    CommandType type;
    virtual ~CommandNode() = default;
};

// search index=main host="server01" error
struct SearchNode : CommandNode {
    std::string index = "*";
    std::string sourcetype;
    std::string host;
    std::string source;
    std::vector<std::string> search_terms;
    uint64_t earliest = 0;
    uint64_t latest = UINT64_MAX;
    std::unique_ptr<Expression> filter;
};

// | where status > 400
struct WhereNode : CommandNode {
    std::unique_ptr<Expression> condition;
};

// | stats count, avg(duration) by host
struct StatsNode : CommandNode {
    std::vector<AggSpec> aggregations;
    std::vector<std::string> group_by;
};

// | table host, source, _raw
struct TableNode : CommandNode {
    std::vector<std::string> fields;
};

// | fields - _raw
struct FieldsNode : CommandNode {
    bool include = true;  // true = keep only these, false = remove these
    std::vector<std::string> fields;
};

// | sort -count, +host
struct SortNode : CommandNode {
    std::vector<std::pair<std::string, SortDir>> fields;
    int limit = 0;
};

// | head 10
struct HeadNode : CommandNode {
    int count = 10;
};

// | tail 10
struct TailNode : CommandNode {
    int count = 10;
};

// | eval new_field = field1 + field2
struct EvalNode : CommandNode {
    std::string output_field;
    std::unique_ptr<Expression> expression;
};

// | timechart span=1h count by sourcetype
struct TimechartNode : CommandNode {
    std::string span;  // e.g., "1h", "5m", "1d"
    std::vector<AggSpec> aggregations;
    std::string split_by;
};

// | dedup host, source
struct DedupNode : CommandNode {
    std::vector<std::string> fields;
    int keep_count = 1;
};

// | rename old_name AS new_name
struct RenameNode : CommandNode {
    std::vector<std::pair<std::string, std::string>> mappings;  // old -> new
};

// | rex field=_raw "(?<ip>\\d+\\.\\d+\\.\\d+\\.\\d+)"
struct RexNode : CommandNode {
    std::string field = "_raw";
    std::string pattern;
};

// Complete SPL query = sequence of piped commands
struct Query {
    std::vector<std::unique_ptr<CommandNode>> commands;
};

} // namespace sentinel::spl
