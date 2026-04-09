#pragma once

#include "common/config.h"
#include "spl/codegen.h"
#include "indexer/segment.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace sentinel::search {

struct SearchResult {
    std::vector<spl::Row> rows;
    std::vector<std::string> columns;
    int64_t total_matched = 0;
    int64_t segments_scanned = 0;
    double execution_time_ms = 0;
};

class Executor {
public:
    static Executor& instance();

    void configure(const common::Config& config);

    // Execute an SPL query string
    SearchResult execute(const std::string& query_string);

    // Register segments for searching
    void register_segment(std::shared_ptr<indexer::Segment> segment);

private:
    Executor() = default;

    common::Config config_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<indexer::Segment>>> segments_;

    SearchResult execute_plan(const spl::ExecutionPlan& plan);
    std::vector<spl::Row> scan_segments(const spl::ScanPlan& scan);
    std::vector<spl::Row> apply_filter(const spl::FilterPlan& filter,
                                        std::vector<spl::Row>& rows);
    std::vector<spl::Row> apply_project(const spl::ProjectPlan& project,
                                         std::vector<spl::Row>& rows);
    std::vector<spl::Row> apply_aggregate(const spl::AggregatePlan& agg,
                                           std::vector<spl::Row>& rows);
    std::vector<spl::Row> apply_sort(const spl::SortPlan& sort,
                                      std::vector<spl::Row>& rows);
    std::vector<spl::Row> apply_limit(const spl::LimitPlan& limit,
                                       std::vector<spl::Row>& rows);
};

} // namespace sentinel::search
