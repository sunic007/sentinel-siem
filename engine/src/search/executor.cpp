#include "search/executor.h"
#include "spl/parser.h"
#include "spl/optimizer.h"
#include "spl/codegen.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <algorithm>
#include <limits>
#include <set>

namespace sentinel::search {

Executor& Executor::instance() {
    static Executor instance;
    return instance;
}

void Executor::configure(const common::Config& config) {
    config_ = config;
}

SearchResult Executor::execute(const std::string& query_string) {
    auto start = std::chrono::high_resolution_clock::now();

    // Parse SPL
    spl::Parser parser(query_string);
    auto query = parser.parse();

    if (parser.has_errors()) {
        SearchResult result;
        for (const auto& err : parser.errors()) {
            spdlog::error("SPL parse error at {}:{}: {}", err.line, err.column, err.message);
        }
        return result;
    }

    // Optimize
    spl::Optimizer optimizer;
    optimizer.optimize(*query);

    // Generate execution plan
    spl::CodeGen codegen;
    auto plan = codegen.generate(*query);

    // Execute
    auto result = execute_plan(plan);

    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    spdlog::info("Query executed in {:.2f}ms: {} results",
                 result.execution_time_ms, result.total_matched);

    return result;
}

void Executor::register_segment(std::shared_ptr<indexer::Segment> segment) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& meta = segment->meta();
    segments_[meta.index_name].push_back(std::move(segment));
}

std::vector<IndexInfo> Executor::list_indexes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IndexInfo> result;

    for (const auto& [name, segs] : segments_) {
        IndexInfo info;
        info.name = name;
        info.segment_count = static_cast<int32_t>(segs.size());
        for (const auto& seg : segs) {
            info.total_events += seg->meta().event_count;
            info.total_size_bytes += seg->meta().size_bytes;
        }
        result.push_back(info);
    }

    return result;
}

SearchResult Executor::execute_plan(const spl::ExecutionPlan& plan) {
    SearchResult result;
    std::vector<spl::Row> rows;

    for (const auto& op : plan.operators) {
        switch (op->type) {
            case spl::OpType::SCAN:
                rows = scan_segments(static_cast<const spl::ScanPlan&>(*op));
                break;
            case spl::OpType::FILTER:
                rows = apply_filter(static_cast<const spl::FilterPlan&>(*op), rows);
                break;
            case spl::OpType::PROJECT:
                rows = apply_project(static_cast<const spl::ProjectPlan&>(*op), rows);
                break;
            case spl::OpType::AGGREGATE:
                rows = apply_aggregate(static_cast<const spl::AggregatePlan&>(*op), rows);
                break;
            case spl::OpType::SORT:
                rows = apply_sort(static_cast<const spl::SortPlan&>(*op), rows);
                break;
            case spl::OpType::LIMIT:
                rows = apply_limit(static_cast<const spl::LimitPlan&>(*op), rows);
                break;
            default:
                break;
        }
    }

    result.rows = std::move(rows);
    result.total_matched = static_cast<int64_t>(result.rows.size());

    // Extract column names
    if (!result.rows.empty()) {
        for (const auto& [key, _] : result.rows[0]) {
            result.columns.push_back(key);
        }
    }

    return result;
}

std::vector<spl::Row> Executor::scan_segments(const spl::ScanPlan& scan) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<spl::Row> rows;

    auto it = segments_.find(scan.index);
    if (it == segments_.end() && scan.index != "*") return rows;

    // Determine which index groups to scan
    std::vector<std::vector<std::shared_ptr<indexer::Segment>>*> scan_groups;
    if (scan.index == "*") {
        for (auto& [name, segs] : segments_) {
            scan_groups.push_back(&segs);
        }
    } else {
        scan_groups.push_back(&it->second);
    }

    for (auto* seg_group : scan_groups) {
        for (const auto& segment : *seg_group) {
            std::vector<indexer::SegmentEvent> events;
            if (scan.search_terms.empty()) {
                events = segment->scan(scan.earliest, scan.latest);
            } else {
                events = segment->search(scan.search_terms, scan.earliest, scan.latest);
            }

            for (const auto& event : events) {
                spl::Row row;
                row["_time"] = std::to_string(event.timestamp_us);
                row["_raw"] = event.raw;
                row["host"] = event.host;
                row["source"] = event.source;
                row["sourcetype"] = event.sourcetype;
                rows.push_back(std::move(row));
            }
        }
    }

    return rows;
}

std::vector<spl::Row> Executor::apply_filter(
    const spl::FilterPlan& filter, std::vector<spl::Row>& rows
) {
    if (!filter.predicate) return rows;

    std::vector<spl::Row> result;
    for (auto& row : rows) {
        if (filter.predicate(row)) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

std::vector<spl::Row> Executor::apply_project(
    const spl::ProjectPlan& project, std::vector<spl::Row>& rows
) {
    if (project.include) {
        for (auto& row : rows) {
            spl::Row filtered;
            for (const auto& field : project.fields) {
                auto it = row.find(field);
                if (it != row.end()) {
                    filtered[field] = std::move(it->second);
                }
            }
            row = std::move(filtered);
        }
    } else {
        for (auto& row : rows) {
            for (const auto& field : project.fields) {
                row.erase(field);
            }
        }
    }
    return rows;
}

std::vector<spl::Row> Executor::apply_aggregate(
    const spl::AggregatePlan& agg, std::vector<spl::Row>& rows
) {
    // Group rows by group_by fields
    std::unordered_map<std::string, std::vector<spl::Row*>> groups;
    for (auto& row : rows) {
        std::string key;
        for (const auto& field : agg.group_by) {
            key += row[field] + "|";
        }
        groups[key].push_back(&row);
    }

    std::vector<spl::Row> results;
    for (auto& [key, group_rows] : groups) {
        spl::Row result_row;

        // Set group-by field values
        if (!group_rows.empty()) {
            for (const auto& field : agg.group_by) {
                result_row[field] = (*group_rows[0])[field];
            }
        }

        // Compute aggregations
        for (const auto& agg_spec : agg.aggregations) {
            switch (agg_spec.func) {
                case spl::AggFunc::COUNT:
                    result_row[agg_spec.alias] = std::to_string(group_rows.size());
                    break;
                case spl::AggFunc::SUM: {
                    double sum = 0;
                    for (auto* r : group_rows) {
                        try { sum += std::stod((*r)[agg_spec.field]); } catch (...) {}
                    }
                    result_row[agg_spec.alias] = std::to_string(sum);
                    break;
                }
                case spl::AggFunc::AVG: {
                    double sum = 0;
                    int count = 0;
                    for (auto* r : group_rows) {
                        try { sum += std::stod((*r)[agg_spec.field]); ++count; } catch (...) {}
                    }
                    result_row[agg_spec.alias] = count > 0 ? std::to_string(sum / count) : "0";
                    break;
                }
                case spl::AggFunc::MIN: {
                    double min_val = std::numeric_limits<double>::max();
                    for (auto* r : group_rows) {
                        try {
                            double v = std::stod((*r)[agg_spec.field]);
                            if (v < min_val) min_val = v;
                        } catch (...) {}
                    }
                    result_row[agg_spec.alias] = std::to_string(min_val);
                    break;
                }
                case spl::AggFunc::MAX: {
                    double max_val = std::numeric_limits<double>::lowest();
                    for (auto* r : group_rows) {
                        try {
                            double v = std::stod((*r)[agg_spec.field]);
                            if (v > max_val) max_val = v;
                        } catch (...) {}
                    }
                    result_row[agg_spec.alias] = std::to_string(max_val);
                    break;
                }
                case spl::AggFunc::DC: {
                    std::set<std::string> unique;
                    for (auto* r : group_rows) {
                        unique.insert((*r)[agg_spec.field]);
                    }
                    result_row[agg_spec.alias] = std::to_string(unique.size());
                    break;
                }
                case spl::AggFunc::VALUES: {
                    std::set<std::string> unique;
                    for (auto* r : group_rows) {
                        unique.insert((*r)[agg_spec.field]);
                    }
                    std::string vals;
                    for (const auto& v : unique) {
                        if (!vals.empty()) vals += ", ";
                        vals += v;
                    }
                    result_row[agg_spec.alias] = vals;
                    break;
                }
                case spl::AggFunc::FIRST: {
                    if (!group_rows.empty()) {
                        result_row[agg_spec.alias] = (*group_rows.front())[agg_spec.field];
                    }
                    break;
                }
                case spl::AggFunc::LAST: {
                    if (!group_rows.empty()) {
                        result_row[agg_spec.alias] = (*group_rows.back())[agg_spec.field];
                    }
                    break;
                }
                default:
                    result_row[agg_spec.alias] = std::to_string(group_rows.size());
                    break;
            }
        }

        results.push_back(std::move(result_row));
    }

    return results;
}

std::vector<spl::Row> Executor::apply_sort(
    const spl::SortPlan& sort, std::vector<spl::Row>& rows
) {
    if (sort.fields.empty()) return rows;

    std::sort(rows.begin(), rows.end(),
        [&sort](const spl::Row& a, const spl::Row& b) {
            for (const auto& [field, dir] : sort.fields) {
                auto ait = a.find(field);
                auto bit = b.find(field);
                std::string av = ait != a.end() ? ait->second : "";
                std::string bv = bit != b.end() ? bit->second : "";

                if (av != bv) {
                    return dir == spl::SortDir::ASC ? av < bv : av > bv;
                }
            }
            return false;
        });

    return rows;
}

std::vector<spl::Row> Executor::apply_limit(
    const spl::LimitPlan& limit, std::vector<spl::Row>& rows
) {
    if (limit.from_end) {
        if (static_cast<size_t>(limit.count) >= rows.size()) return rows;
        return {rows.end() - limit.count, rows.end()};
    } else {
        if (static_cast<size_t>(limit.count) >= rows.size()) return rows;
        rows.resize(limit.count);
        return rows;
    }
}

} // namespace sentinel::search
