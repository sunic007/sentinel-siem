#pragma once

#include "common/config.h"
#include "indexer/wal.h"
#include "indexer/segment.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace sentinel::ingest {

struct IngestEvent {
    std::string raw;
    std::string host;
    std::string source;
    std::string sourcetype;
    std::string index = "main";
    uint64_t timestamp_us = 0;
};

class Pipeline {
public:
    static Pipeline& instance();

    void configure(const common::Config& config);
    void start();
    void stop();

    // Ingest a batch of events
    int64_t ingest(const std::vector<IngestEvent>& events);

    // Get statistics
    int64_t total_events() const { return total_events_.load(); }
    int64_t events_per_second() const;

private:
    Pipeline() = default;

    common::Config config_;
    std::unique_ptr<indexer::Wal> wal_;
    std::atomic<bool> running_{false};
    std::atomic<int64_t> total_events_{0};
    std::atomic<int64_t> events_last_second_{0};

    std::vector<indexer::SegmentEvent> buffer_;
    size_t buffer_limit_ = 10000;
    std::jthread flush_thread_;

    void flush_buffer();
    uint64_t extract_timestamp(const std::string& raw);
};

} // namespace sentinel::ingest
