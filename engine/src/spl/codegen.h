#pragma once

#include "spl/ast.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

namespace sentinel::spl {

// Execution plan operator types
enum class OpType {
    SCAN,       // Read events from segments
    FILTER,     // Apply predicate
    PROJECT,    // Select/rename fields
    AGGREGATE,  // Stats/timechart aggregation
    SORT,       // Sort results
    LIMIT,      // Head/tail
    DEDUP,      // Deduplicate
    EVAL        // Compute new fields
};

// A row in the result set
using Row = std::unordered_map<std::string, std::string>;

// Execution plan node
struct PlanNode {
    OpType type;
    virtual ~PlanNode() = default;
};

struct ScanPlan : PlanNode {
    std::string index;
    std::vector<std::string> search_terms;
    uint64_t earliest = 0;
    uint64_t latest = UINT64_MAX;
};

struct FilterPlan : PlanNode {
    std::function<bool(const Row&)> predicate;
};

struct ProjectPlan : PlanNode {
    std::vector<std::string> fields;
    bool include = true;
};

struct AggregatePlan : PlanNode {
    std::vector<AggSpec> aggregations;
    std::vector<std::string> group_by;
};

struct SortPlan : PlanNode {
    std::vector<std::pair<std::string, SortDir>> fields;
};

struct LimitPlan : PlanNode {
    int count;
    bool from_end = false;  // true for tail
};

// Execution plan: linear pipeline of operators
struct ExecutionPlan {
    std::vector<std::unique_ptr<PlanNode>> operators;
};

// Code generator: transforms AST into execution plan
class CodeGen {
public:
    ExecutionPlan generate(const Query& query);

private:
    void generate_command(const CommandNode& cmd, ExecutionPlan& plan);
};

} // namespace sentinel::spl
