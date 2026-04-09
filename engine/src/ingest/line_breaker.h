#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace sentinel::ingest {

// Breaks raw input data into individual events
class LineBreaker {
public:
    enum class Mode {
        SINGLE_LINE,     // Each line is an event
        MULTI_LINE,      // Events span multiple lines (e.g., Java stack traces)
        JSON_ARRAY,      // JSON array of events
        AUTO             // Auto-detect
    };

    explicit LineBreaker(Mode mode = Mode::AUTO);

    // Break raw data into individual events
    std::vector<std::string> break_events(std::string_view data);

    // Set the multi-line break pattern (regex)
    void set_break_pattern(const std::string& pattern);

private:
    Mode mode_;
    std::string break_pattern_;

    std::vector<std::string> break_single_line(std::string_view data);
    std::vector<std::string> break_multi_line(std::string_view data);
    std::vector<std::string> break_json_array(std::string_view data);
    Mode detect_mode(std::string_view data);
};

} // namespace sentinel::ingest
