#pragma once

#include <string>

namespace sentinel::common {

void init_logging(const std::string& logger_name, const std::string& log_level = "info");

} // namespace sentinel::common
