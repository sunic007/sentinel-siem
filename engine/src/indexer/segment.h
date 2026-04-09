#pragma once

#include "indexer/inverted_index.h"
#include "indexer/bloom_filter.h"
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <memory>

namespace sentinel::indexer {

// Event stored in a segment
struct SegmentEvent {
    uint64_t id;
    uint64_t timestamp_us;
    std::string raw;
    std::string host;
    std::string source;
    std::string sourcetype;
};

// Segment metadata
struct SegmentMeta {
    std::string segment_id;
    std::string index_name;
    uint64_t event_count = 0;
    uint64_t size_bytes = 0;
    uint64_t earliest_time = 0;
    uint64_t latest_time = 0;
    std::string state = "hot";  // hot, warm, cold, frozen
};

// Immutable segment on disk containing indexed events
class Segment {
public:
    // Create a new segment for writing
    static std::unique_ptr<Segment> create(
        const std::filesystem::path& dir,
        const std::string& index_name,
        const std::string& segment_id
    );

    // Open an existing segment for reading
    static std::unique_ptr<Segment> open(const std::filesystem::path& dir);

    // Write events to the segment and seal it
    void write_events(const std::vector<SegmentEvent>& events);

    // Search for events matching terms within a time range
    std::vector<SegmentEvent> search(
        const std::vector<std::string>& terms,
        uint64_t earliest = 0,
        uint64_t latest = UINT64_MAX
    ) const;

    // Check if a term might exist in this segment (bloom filter)
    bool maybe_contains(const std::string& term) const;

    // Get all events in time range
    std::vector<SegmentEvent> scan(
        uint64_t earliest = 0,
        uint64_t latest = UINT64_MAX
    ) const;

    const SegmentMeta& meta() const { return meta_; }
    bool is_sealed() const { return sealed_; }

private:
    Segment() = default;

    SegmentMeta meta_;
    std::filesystem::path dir_;
    bool sealed_ = false;

    // In-memory data (loaded on demand)
    mutable std::vector<SegmentEvent> events_;
    mutable std::unique_ptr<InvertedIndex> inverted_index_;
    mutable std::unique_ptr<BloomFilter> bloom_filter_;

    void build_index();
    void save_to_disk();
    void load_from_disk() const;
};

} // namespace sentinel::indexer
