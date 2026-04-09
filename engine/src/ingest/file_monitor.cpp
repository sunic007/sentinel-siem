#include "ingest/file_monitor.h"
#include <spdlog/spdlog.h>
#include <fstream>

namespace sentinel::ingest {

FileMonitor::FileMonitor(Pipeline& pipeline) : pipeline_(pipeline) {}

FileMonitor::~FileMonitor() {
    stop();
}

void FileMonitor::add_input(const FileMonitorConfig& config) {
    configs_.push_back(config);
    spdlog::info("File monitor input added: {}", config.path.string());
}

void FileMonitor::start() {
    running_ = true;
    monitor_thread_ = std::jthread([this](std::stop_token token) {
        monitor_loop();
    });
    spdlog::info("File monitor started with {} inputs", configs_.size());
}

void FileMonitor::stop() {
    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.request_stop();
        monitor_thread_.join();
    }
}

void FileMonitor::monitor_loop() {
    while (running_) {
        for (const auto& config : configs_) {
            if (std::filesystem::is_regular_file(config.path)) {
                process_file(config.path, config);
            } else if (std::filesystem::is_directory(config.path)) {
                for (const auto& entry : std::filesystem::directory_iterator(config.path)) {
                    if (entry.is_regular_file()) {
                        process_file(entry.path(), config);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void FileMonitor::process_file(const std::filesystem::path& path,
                                const FileMonitorConfig& config) {
    std::string path_str = path.string();

    // Get current position
    auto& pos = file_positions_[path_str];

    std::ifstream file(path);
    if (!file) return;

    // Seek to last known position
    file.seekg(pos);

    std::vector<IngestEvent> events;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        IngestEvent event;
        event.raw = line;
        event.source = path_str;
        event.sourcetype = config.sourcetype == "auto"
            ? detect_sourcetype(path)
            : config.sourcetype;
        event.index = config.index;
        events.push_back(std::move(event));
    }

    // Update position
    file.clear();
    pos = file.tellg();

    // Ingest
    if (!events.empty()) {
        pipeline_.ingest(events);
        spdlog::debug("Read {} lines from {}", events.size(), path_str);
    }
}

std::string FileMonitor::detect_sourcetype(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    auto filename = path.filename().string();

    if (filename == "auth.log" || filename == "secure") return "linux_secure";
    if (filename == "syslog" || filename == "messages") return "syslog";
    if (filename == "access.log") return "access_combined";
    if (filename == "error.log") return "error_log";
    if (ext == ".json") return "json";
    if (ext == ".csv") return "csv";

    return "generic_single_line";
}

} // namespace sentinel::ingest
