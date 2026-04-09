#include "spl/semantic.h"

namespace sentinel::spl {

std::vector<SemanticError> SemanticAnalyzer::analyze(const Query& query) {
    std::vector<SemanticError> errors;

    if (query.commands.empty()) {
        errors.push_back({"Query must have at least one command"});
        return errors;
    }

    for (const auto& cmd : query.commands) {
        validate_command(*cmd, errors);
    }

    return errors;
}

void SemanticAnalyzer::validate_command(const CommandNode& cmd,
                                         std::vector<SemanticError>& errors) {
    switch (cmd.type) {
        case CommandType::STATS: {
            auto& stats = static_cast<const StatsNode&>(cmd);
            if (stats.aggregations.empty()) {
                errors.push_back({"stats requires at least one aggregation"});
            }
            break;
        }
        case CommandType::TABLE: {
            auto& table = static_cast<const TableNode&>(cmd);
            if (table.fields.empty()) {
                errors.push_back({"table requires at least one field"});
            }
            break;
        }
        case CommandType::SORT: {
            auto& sort = static_cast<const SortNode&>(cmd);
            if (sort.fields.empty()) {
                errors.push_back({"sort requires at least one field"});
            }
            break;
        }
        default:
            break;
    }
}

std::vector<std::string> SemanticAnalyzer::referenced_fields(const Query& query) const {
    std::vector<std::string> fields;
    // TODO: Walk AST and collect all field references
    return fields;
}

std::vector<std::string> SemanticAnalyzer::referenced_indexes(const Query& query) const {
    std::vector<std::string> indexes;

    for (const auto& cmd : query.commands) {
        if (cmd->type == CommandType::SEARCH) {
            auto& search = static_cast<const SearchNode&>(*cmd);
            if (search.index != "*") {
                indexes.push_back(search.index);
            }
        }
    }

    return indexes;
}

} // namespace sentinel::spl
