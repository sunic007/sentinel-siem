#pragma once

#include "spl/ast.h"
#include "spl/codegen.h"

namespace sentinel::search {

// Streaming aggregator for stats/timechart commands
class Aggregator {
public:
    void configure(const std::vector<spl::AggSpec>& aggregations,
                   const std::vector<std::string>& group_by);
    void add_row(const spl::Row& row);
    std::vector<spl::Row> finalize();

private:
    std::vector<spl::AggSpec> aggregations_;
    std::vector<std::string> group_by_;
};

} // namespace sentinel::search
