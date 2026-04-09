#include "ingest/timestamp_extractor.h"
#include <regex>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace sentinel::ingest {

uint64_t TimestampExtractor::extract(std::string_view text) {
    // Try ISO 8601 first (most precise)
    // Pattern: 2024-01-15T10:30:00.000Z or 2024-01-15T10:30:00+05:00
    static const std::regex iso_regex(
        R"((\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?))"
    );

    // Try epoch timestamps
    static const std::regex epoch_regex(R"(\b(1\d{9}(?:\.\d+)?)\b)");

    // Try syslog format: Jan 15 10:30:00
    static const std::regex syslog_regex(
        R"(([A-Z][a-z]{2}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2}))"
    );

    std::string text_str(text);
    std::smatch match;

    if (std::regex_search(text_str, match, iso_regex)) {
        return parse_iso8601(match[1].str());
    }

    if (std::regex_search(text_str, match, epoch_regex)) {
        return parse_epoch(match[1].str());
    }

    if (std::regex_search(text_str, match, syslog_regex)) {
        return parse_syslog(match[1].str());
    }

    return 0;
}

uint64_t TimestampExtractor::parse_iso8601(std::string_view ts) {
    std::tm tm = {};
    std::istringstream ss(std::string(ts));

    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        ss.clear();
        ss.str(std::string(ts));
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) return 0;
    }

    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::microseconds>(
        time_point.time_since_epoch()
    ).count();
}

uint64_t TimestampExtractor::parse_epoch(std::string_view ts) {
    try {
        double epoch = std::stod(std::string(ts));
        return static_cast<uint64_t>(epoch * 1'000'000);
    } catch (...) {
        return 0;
    }
}

uint64_t TimestampExtractor::parse_syslog(std::string_view ts) {
    std::tm tm = {};
    std::istringstream ss(std::string(ts));
    ss >> std::get_time(&tm, "%b %d %H:%M:%S");
    if (ss.fail()) return 0;

    // Syslog timestamps don't include year, use current year
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto* now_tm = std::localtime(&now_time);
    tm.tm_year = now_tm->tm_year;

    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::microseconds>(
        time_point.time_since_epoch()
    ).count();
}

uint64_t TimestampExtractor::parse_common_log(std::string_view ts) {
    // Apache common log format: [15/Jan/2024:10:30:00 +0500]
    std::tm tm = {};
    std::istringstream ss(std::string(ts));
    ss >> std::get_time(&tm, "%d/%b/%Y:%H:%M:%S");
    if (ss.fail()) return 0;

    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::microseconds>(
        time_point.time_since_epoch()
    ).count();
}

} // namespace sentinel::ingest
