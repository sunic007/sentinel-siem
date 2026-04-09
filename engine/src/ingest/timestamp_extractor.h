#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace sentinel::ingest {

class TimestampExtractor {
public:
    // Extract timestamp from raw event text, returns microseconds since epoch
    // Returns 0 if no timestamp found
    static uint64_t extract(std::string_view text);

    // Common timestamp formats
    static uint64_t parse_iso8601(std::string_view ts);
    static uint64_t parse_syslog(std::string_view ts);
    static uint64_t parse_epoch(std::string_view ts);
    static uint64_t parse_common_log(std::string_view ts);
};

} // namespace sentinel::ingest
