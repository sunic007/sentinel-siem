#include "common/config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>

namespace sentinel::common {

Config Config::defaults() {
    return Config{};
}

Config Config::from_file(const std::string& path) {
    Config config;

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::warn("Config file not found: {}, using defaults", path);
        return config;
    }

    try {
        auto json = nlohmann::json::parse(file);

        if (json.contains("data_dir"))
            config.data_dir_ = json["data_dir"].get<std::string>();
        if (json.contains("listen_address"))
            config.listen_address_ = json["listen_address"].get<std::string>();
        if (json.contains("grpc_port"))
            config.grpc_port_ = json["grpc_port"].get<uint16_t>();
        if (json.contains("http_port"))
            config.http_port_ = json["http_port"].get<uint16_t>();
        if (json.contains("segment_size_mb"))
            config.segment_size_mb_ = json["segment_size_mb"].get<size_t>();
        if (json.contains("compression_level"))
            config.compression_level_ = json["compression_level"].get<int>();
        if (json.contains("syslog_udp_port"))
            config.syslog_udp_port_ = json["syslog_udp_port"].get<uint16_t>();
        if (json.contains("syslog_tcp_port"))
            config.syslog_tcp_port_ = json["syslog_tcp_port"].get<uint16_t>();
        if (json.contains("search_threads"))
            config.search_threads_ = json["search_threads"].get<size_t>();
        if (json.contains("ingest_threads"))
            config.ingest_threads_ = json["ingest_threads"].get<size_t>();

        spdlog::info("Loaded config from {}", path);
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse config {}: {}", path, e.what());
    }

    return config;
}

} // namespace sentinel::common
