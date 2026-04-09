#include "spl/optimizer.h"
#include <spdlog/spdlog.h>

namespace sentinel::spl {

void Optimizer::optimize(Query& query) {
    push_down_filters(query);
    prune_unused_fields(query);
    extract_time_range(query);
    spdlog::debug("Query optimized: {} commands", query.commands.size());
}

void Optimizer::push_down_filters(Query& query) {
    // Move WHERE clauses as early as possible in the pipeline
    // to reduce data flowing through later stages
    // TODO: Implement predicate pushdown
}

void Optimizer::prune_unused_fields(Query& query) {
    // If a TABLE or FIELDS command limits output fields,
    // propagate that information backward so earlier stages
    // can skip extracting unused fields
    // TODO: Implement field pruning
}

void Optimizer::extract_time_range(Query& query) {
    // Extract time range from search expressions to enable
    // segment-level pruning
    // TODO: Implement time range extraction
}

} // namespace sentinel::spl
