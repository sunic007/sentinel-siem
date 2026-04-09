#include "ingest/line_breaker.h"
#include <sstream>

namespace sentinel::ingest {

LineBreaker::LineBreaker(Mode mode) : mode_(mode) {}

std::vector<std::string> LineBreaker::break_events(std::string_view data) {
    Mode actual_mode = mode_ == Mode::AUTO ? detect_mode(data) : mode_;

    switch (actual_mode) {
        case Mode::SINGLE_LINE: return break_single_line(data);
        case Mode::MULTI_LINE:  return break_multi_line(data);
        case Mode::JSON_ARRAY:  return break_json_array(data);
        default:                return break_single_line(data);
    }
}

void LineBreaker::set_break_pattern(const std::string& pattern) {
    break_pattern_ = pattern;
}

std::vector<std::string> LineBreaker::break_single_line(std::string_view data) {
    std::vector<std::string> events;
    std::istringstream stream(std::string(data));
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty()) {
            // Remove trailing CR if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            events.push_back(std::move(line));
        }
    }

    return events;
}

std::vector<std::string> LineBreaker::break_multi_line(std::string_view data) {
    // For multi-line events (e.g., Java stack traces):
    // Lines starting with whitespace/tab are continuations
    std::vector<std::string> events;
    std::string current_event;
    std::istringstream stream(std::string(data));
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (!line.empty() && (line[0] == ' ' || line[0] == '\t') && !current_event.empty()) {
            current_event += "\n" + line;
        } else {
            if (!current_event.empty()) {
                events.push_back(std::move(current_event));
            }
            current_event = line;
        }
    }

    if (!current_event.empty()) {
        events.push_back(std::move(current_event));
    }

    return events;
}

std::vector<std::string> LineBreaker::break_json_array(std::string_view data) {
    // Simple JSON array splitting (one JSON object per event)
    // For production, use a proper JSON streaming parser
    std::vector<std::string> events;
    int depth = 0;
    std::string current;

    for (char c : data) {
        if (c == '{') {
            if (depth == 0) current.clear();
            ++depth;
            current += c;
        } else if (c == '}') {
            current += c;
            --depth;
            if (depth == 0) {
                events.push_back(std::move(current));
                current.clear();
            }
        } else if (depth > 0) {
            current += c;
        }
    }

    return events;
}

LineBreaker::Mode LineBreaker::detect_mode(std::string_view data) {
    if (!data.empty() && data[0] == '[') return Mode::JSON_ARRAY;
    if (data.find("\n\t") != std::string_view::npos ||
        data.find("\n ") != std::string_view::npos) {
        return Mode::MULTI_LINE;
    }
    return Mode::SINGLE_LINE;
}

} // namespace sentinel::ingest
