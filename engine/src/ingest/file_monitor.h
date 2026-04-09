#pragma once

#include "ingest/pipeline.h"
#include <string>
#include <vector>
#include <filesystem>
#include <atomic>
#include <thread>
#include <unordered_map>

namespace sentinel::ingest {

struct FileMonitorConfig {
    std::filesystem::path path;       // File or directory to monitor
    std::string sourcetype = "auto";  // Source type
    std::string index = "main";
    bool recursive = false;
    std::string whitelist;            // Regex for filename filter
    std::string blacklist;            // Regex for filename exclusion
};

class FileMonitor {
public:
    FileMonitor(Pipeline& pipeline);
    ~FileMonitor();

    void add_input(const FileMonitorConfig& config);
    void start();
    void stop();

private:
    Pipeline& pipeline_;
    std::vector<FileMonitorConfig> configs_;
    std::atomic<bool> running_{false};
    std::jthread monitor_thread_;

    // Track file read positions
    std::unordered_map<std::string, std::streampos> file_positions_;

    void monitor_loop();
    void process_file(const std::filesystem::path& path, const FileMonitorConfig& config);
    std::string detect_sourcetype(const std::filesystem::path& path);
};

} // namespace sentinel::ingest
