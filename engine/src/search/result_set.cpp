#include "search/result_set.h"
#include <nlohmann/json.hpp>

namespace sentinel::search {

void ResultSet::add_row(spl::Row row) {
    rows_.push_back(std::move(row));
}

void ResultSet::set_columns(std::vector<std::string> columns) {
    columns_ = std::move(columns);
}

std::string ResultSet::to_json() const {
    nlohmann::json json;
    json["columns"] = columns_;
    json["rows"] = nlohmann::json::array();

    for (const auto& row : rows_) {
        nlohmann::json row_json;
        for (const auto& [key, value] : row) {
            row_json[key] = value;
        }
        json["rows"].push_back(row_json);
    }

    json["total"] = rows_.size();
    return json.dump();
}

} // namespace sentinel::search
