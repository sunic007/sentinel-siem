#include "spl/codegen.h"
#include <spdlog/spdlog.h>

namespace sentinel::spl {

ExecutionPlan CodeGen::generate(const Query& query) {
    ExecutionPlan plan;

    for (const auto& cmd : query.commands) {
        generate_command(*cmd, plan);
    }

    spdlog::debug("Generated execution plan with {} operators", plan.operators.size());
    return plan;
}

void CodeGen::generate_command(const CommandNode& cmd, ExecutionPlan& plan) {
    switch (cmd.type) {
        case CommandType::SEARCH: {
            auto& search = static_cast<const SearchNode&>(cmd);
            auto scan = std::make_unique<ScanPlan>();
            scan->type = OpType::SCAN;
            scan->index = search.index;
            scan->search_terms = search.search_terms;
            scan->earliest = search.earliest;
            scan->latest = search.latest;
            plan.operators.push_back(std::move(scan));
            break;
        }
        case CommandType::WHERE: {
            auto filter = std::make_unique<FilterPlan>();
            filter->type = OpType::FILTER;
            // TODO: Compile expression to predicate function
            plan.operators.push_back(std::move(filter));
            break;
        }
        case CommandType::TABLE: {
            auto& table = static_cast<const TableNode&>(cmd);
            auto project = std::make_unique<ProjectPlan>();
            project->type = OpType::PROJECT;
            project->fields = table.fields;
            project->include = true;
            plan.operators.push_back(std::move(project));
            break;
        }
        case CommandType::FIELDS: {
            auto& fields = static_cast<const FieldsNode&>(cmd);
            auto project = std::make_unique<ProjectPlan>();
            project->type = OpType::PROJECT;
            project->fields = fields.fields;
            project->include = fields.include;
            plan.operators.push_back(std::move(project));
            break;
        }
        case CommandType::STATS:
        case CommandType::TIMECHART: {
            auto& stats = static_cast<const StatsNode&>(cmd);
            auto agg = std::make_unique<AggregatePlan>();
            agg->type = OpType::AGGREGATE;
            agg->aggregations = stats.aggregations;
            agg->group_by = stats.group_by;
            plan.operators.push_back(std::move(agg));
            break;
        }
        case CommandType::SORT: {
            auto& sort_cmd = static_cast<const SortNode&>(cmd);
            auto sort = std::make_unique<SortPlan>();
            sort->type = OpType::SORT;
            sort->fields = sort_cmd.fields;
            plan.operators.push_back(std::move(sort));
            break;
        }
        case CommandType::HEAD: {
            auto& head = static_cast<const HeadNode&>(cmd);
            auto limit = std::make_unique<LimitPlan>();
            limit->type = OpType::LIMIT;
            limit->count = head.count;
            limit->from_end = false;
            plan.operators.push_back(std::move(limit));
            break;
        }
        case CommandType::TAIL: {
            auto& tail = static_cast<const TailNode&>(cmd);
            auto limit = std::make_unique<LimitPlan>();
            limit->type = OpType::LIMIT;
            limit->count = tail.count;
            limit->from_end = true;
            plan.operators.push_back(std::move(limit));
            break;
        }
        default:
            spdlog::warn("Unhandled command type in code generation");
            break;
    }
}

} // namespace sentinel::spl
