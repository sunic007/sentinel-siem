#pragma once

#include "spl/ast.h"
#include <vector>
#include <string>

namespace sentinel::spl {

struct SemanticError {
    std::string message;
};

class SemanticAnalyzer {
public:
    // Validate a parsed query
    std::vector<SemanticError> analyze(const Query& query);

    // Get all referenced fields
    std::vector<std::string> referenced_fields(const Query& query) const;

    // Get all referenced indexes
    std::vector<std::string> referenced_indexes(const Query& query) const;

private:
    void validate_command(const CommandNode& cmd, std::vector<SemanticError>& errors);
};

} // namespace sentinel::spl
