#pragma once

#include <string>
#include <cstdint>

namespace sentinel::common {

class Config {
public:
    static Config defaults();
    static Config from_file(const std::string& path);

    const std::string& data_dir() const { return data_dir_; }
    const std::string& listen_address() const { return listen_address_; }
    uint16_t grpc_port() const { return grpc_port_; }
    uint16_t http_port() const { return http_port_; }

    // Indexer settings
    size_t segment_size_mb() const { return segment_size_mb_; }
    size_t wal_buffer_size() const { return wal_buffer_size_; }
    int compression_level() const { return compression_level_; }

    // Ingest settings
    uint16_t syslog_udp_port() const { return syslog_udp_port_; }
    uint16_t syslog_tcp_port() const { return syslog_tcp_port_; }

    // Thread pool
    size_t search_threads() const { return search_threads_; }
    size_t ingest_threads() const { return ingest_threads_; }

private:
    std::string data_dir_ = "./data";
    std::string listen_address_ = "0.0.0.0";
    uint16_t grpc_port_ = 50051;
    uint16_t http_port_ = 8080;
    size_t segment_size_mb_ = 256;
    size_t wal_buffer_size_ = 16 * 1024 * 1024;  // 16MB
    int compression_level_ = 3;
    uint16_t syslog_udp_port_ = 514;
    uint16_t syslog_tcp_port_ = 514;
    size_t search_threads_ = 4;
    size_t ingest_threads_ = 2;
};

} // namespace sentinel::common
