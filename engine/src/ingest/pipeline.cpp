#include "ingest/pipeline.h"
#include "ingest/timestamp_extractor.h"
#include "search/executor.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace sentinel::ingest {

Pipeline& Pipeline::instance() {
    static Pipeline instance;
    return instance;
}

void Pipeline::configure(const common::Config& config) {
    config_ = config;
    auto wal_dir = std::filesystem::path(config.data_dir()) / "wal";
    wal_ = std::make_unique<indexer::Wal>(wal_dir, config.wal_buffer_size());
}

void Pipeline::start() {
    running_ = true;

    // Background flush thread
    flush_thread_ = std::jthread([this](std::stop_token token) {
        while (!token.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!buffer_.empty()) {
                flush_buffer();
            }
        }
    });

    spdlog::info("Ingest pipeline started");
}

void Pipeline::stop() {
    running_ = false;
    if (flush_thread_.joinable()) {
        flush_thread_.request_stop();
        flush_thread_.join();
    }

    // Flush remaining
    if (!buffer_.empty()) {
        flush_buffer();
    }

    spdlog::info("Ingest pipeline stopped. Total events: {}", total_events_.load());
}

int64_t Pipeline::ingest(const std::vector<IngestEvent>& events) {
    int64_t accepted = 0;

    for (const auto& event : events) {
        // Write to WAL for durability
        indexer::WalEntry wal_entry;
        wal_entry.timestamp_us = event.timestamp_us > 0
            ? event.timestamp_us
            : extract_timestamp(event.raw);
        wal_entry.index_name = event.index;
        wal_entry.raw_event = event.raw;
        wal_entry.host = event.host;
        wal_entry.source = event.source;
        wal_entry.sourcetype = event.sourcetype;

        wal_->append(wal_entry);

        // Add to in-memory buffer
        indexer::SegmentEvent seg_event;
        seg_event.id = total_events_.load();
        seg_event.timestamp_us = wal_entry.timestamp_us;
        seg_event.raw = event.raw;
        seg_event.host = event.host;
        seg_event.source = event.source;
        seg_event.sourcetype = event.sourcetype;

        buffer_.push_back(std::move(seg_event));
        ++accepted;
        total_events_.fetch_add(1);
    }

    // Flush if buffer is full
    if (buffer_.size() >= buffer_limit_) {
        flush_buffer();
    }

    return accepted;
}

void Pipeline::flush_buffer() {
    if (buffer_.empty()) return;

    // Determine index from the first event (all events in a batch share the same index for now)
    std::string index_name = "main";

    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::string segment_id = "seg_" + std::to_string(ms);

    auto data_dir = std::filesystem::path(config_.data_dir()) / "indexes" / index_name;
    auto segment = indexer::Segment::create(data_dir, index_name, segment_id);
    segment->write_events(buffer_);

    // Register segment with search executor so it's immediately searchable
    auto shared_seg = std::shared_ptr<indexer::Segment>(std::move(segment));
    search::Executor::instance().register_segment(shared_seg);

    spdlog::info("Flushed {} events to segment {} (index={})", buffer_.size(), segment_id, index_name);
    buffer_.clear();
}

uint64_t Pipeline::extract_timestamp(const std::string& raw) {
    auto ts = TimestampExtractor::extract(raw);
    if (ts > 0) return ts;

    // Default: current time
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

int64_t Pipeline::events_per_second() const {
    return events_last_second_.load();
}

} // namespace sentinel::ingest
