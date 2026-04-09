#include "search/aggregator.h"

namespace sentinel::search {

void Aggregator::configure(const std::vector<spl::AggSpec>& aggregations,
                           const std::vector<std::string>& group_by) {
    aggregations_ = aggregations;
    group_by_ = group_by;
}

void Aggregator::add_row(const spl::Row& row) {
    // TODO: Streaming aggregation implementation
}

std::vector<spl::Row> Aggregator::finalize() {
    // TODO: Return aggregated results
    return {};
}

} // namespace sentinel::search
