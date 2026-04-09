#pragma once

#include "spl/codegen.h"
#include <vector>
#include <string>

namespace sentinel::search {

// Streaming result set for search results
class ResultSet {
public:
    void add_row(spl::Row row);
    void set_columns(std::vector<std::string> columns);

    const std::vector<spl::Row>& rows() const { return rows_; }
    const std::vector<std::string>& columns() const { return columns_; }
    size_t size() const { return rows_.size(); }
    bool empty() const { return rows_.empty(); }

    // Serialize to JSON
    std::string to_json() const;

private:
    std::vector<spl::Row> rows_;
    std::vector<std::string> columns_;
};

} // namespace sentinel::search
