#pragma once

#include "spl/ast.h"
#include <memory>

namespace sentinel::spl {

// Query plan optimizer
class Optimizer {
public:
    // Optimize a query AST (predicate pushdown, field pruning, etc.)
    void optimize(Query& query);

private:
    void push_down_filters(Query& query);
    void prune_unused_fields(Query& query);
    void extract_time_range(Query& query);
};

} // namespace sentinel::spl
