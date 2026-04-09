#include "indexer/segment.h"
#include "storage/compression.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace sentinel::indexer {

std::unique_ptr<Segment> Segment::create(
    const std::filesystem::path& dir,
    const std::string& index_name,
    const std::string& segment_id
) {
    auto segment = std::unique_ptr<Segment>(new Segment());
    segment->dir_ = dir / segment_id;
    segment->meta_.segment_id = segment_id;
    segment->meta_.index_name = index_name;

    std::filesystem::create_directories(segment->dir_);
    return segment;
}

std::unique_ptr<Segment> Segment::open(const std::filesystem::path& dir) {
    auto meta_path = dir / "segment.meta";
    if (!std::filesystem::exists(meta_path)) return nullptr;

    auto segment = std::unique_ptr<Segment>(new Segment());
    segment->dir_ = dir;
    segment->sealed_ = true;

    // Load metadata
    std::ifstream meta_file(meta_path);
    auto json = nlohmann::json::parse(meta_file);

    segment->meta_.segment_id = json["segment_id"].get<std::string>();
    segment->meta_.index_name = json["index_name"].get<std::string>();
    segment->meta_.event_count = json["event_count"].get<uint64_t>();
    segment->meta_.size_bytes = json["size_bytes"].get<uint64_t>();
    segment->meta_.earliest_time = json["earliest_time"].get<uint64_t>();
    segment->meta_.latest_time = json["latest_time"].get<uint64_t>();
    segment->meta_.state = json.value("state", "warm");

    return segment;
}

void Segment::write_events(const std::vector<SegmentEvent>& events) {
    if (sealed_) {
        spdlog::error("Cannot write to sealed segment {}", meta_.segment_id);
        return;
    }

    events_ = events;
    meta_.event_count = events.size();

    if (!events.empty()) {
        // Sort by timestamp
        std::sort(events_.begin(), events_.end(),
                  [](const auto& a, const auto& b) {
                      return a.timestamp_us < b.timestamp_us;
                  });

        meta_.earliest_time = events_.front().timestamp_us;
        meta_.latest_time = events_.back().timestamp_us;
    }

    build_index();
    save_to_disk();
    sealed_ = true;

    spdlog::info("Segment {} sealed: {} events, {} bytes",
                 meta_.segment_id, meta_.event_count, meta_.size_bytes);
}

bool Segment::maybe_contains(const std::string& term) const {
    if (!bloom_filter_) load_from_disk();
    return bloom_filter_ && bloom_filter_->possibly_contains(term);
}

std::vector<SegmentEvent> Segment::search(
    const std::vector<std::string>& terms,
    uint64_t earliest, uint64_t latest
) const {
    // Quick bloom filter check
    for (const auto& term : terms) {
        if (!maybe_contains(term)) return {};
    }

    load_from_disk();
    if (!inverted_index_) return {};

    // Get matching event IDs from inverted index
    auto matching_ids = inverted_index_->search_and(terms);

    // Filter by time range and collect events
    std::vector<SegmentEvent> results;
    for (uint64_t id : matching_ids) {
        if (id < events_.size()) {
            const auto& event = events_[id];
            if (event.timestamp_us >= earliest && event.timestamp_us <= latest) {
                results.push_back(event);
            }
        }
    }

    return results;
}

std::vector<SegmentEvent> Segment::scan(
    uint64_t earliest, uint64_t latest
) const {
    // Time range check on metadata first
    if (meta_.latest_time < earliest || meta_.earliest_time > latest) {
        return {};
    }

    load_from_disk();

    std::vector<SegmentEvent> results;
    for (const auto& event : events_) {
        if (event.timestamp_us >= earliest && event.timestamp_us <= latest) {
            results.push_back(event);
        }
    }
    return results;
}

void Segment::build_index() {
    inverted_index_ = std::make_unique<InvertedIndex>();
    for (size_t i = 0; i < events_.size(); ++i) {
        inverted_index_->index_event(i, events_[i].raw);
    }

    // Build bloom filter from all terms
    auto terms = inverted_index_->all_terms();
    bloom_filter_ = std::make_unique<BloomFilter>(terms.size());
    for (const auto& term : terms) {
        bloom_filter_->add(term);
    }
}

void Segment::save_to_disk() {
    // Save metadata
    nlohmann::json meta_json = {
        {"segment_id", meta_.segment_id},
        {"index_name", meta_.index_name},
        {"event_count", meta_.event_count},
        {"size_bytes", meta_.size_bytes},
        {"earliest_time", meta_.earliest_time},
        {"latest_time", meta_.latest_time},
        {"state", meta_.state}
    };

    std::ofstream meta_file(dir_ / "segment.meta");
    meta_file << meta_json.dump(2);

    // Save events (compressed)
    nlohmann::json events_json = nlohmann::json::array();
    for (const auto& event : events_) {
        events_json.push_back({
            {"id", event.id},
            {"t", event.timestamp_us},
            {"r", event.raw},
            {"h", event.host},
            {"s", event.source},
            {"st", event.sourcetype}
        });
    }

    std::string events_str = events_json.dump();
    auto compressed = storage::Compression::compress_string(events_str);

    std::ofstream events_file(dir_ / "events.dat", std::ios::binary);
    events_file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());

    meta_.size_bytes = compressed.size();

    // Save inverted index
    if (inverted_index_) {
        auto index_data = inverted_index_->serialize();
        std::ofstream index_file(dir_ / "index.idx", std::ios::binary);
        index_file.write(reinterpret_cast<const char*>(index_data.data()), index_data.size());
    }

    // Save bloom filter
    if (bloom_filter_) {
        auto bloom_data = bloom_filter_->serialize();
        std::ofstream bloom_file(dir_ / "bloom.bf", std::ios::binary);
        uint32_t num_hashes = static_cast<uint32_t>(bloom_filter_->num_hashes());
        bloom_file.write(reinterpret_cast<const char*>(&num_hashes), sizeof(uint32_t));
        bloom_file.write(reinterpret_cast<const char*>(bloom_data.data()), bloom_data.size());
    }
}

void Segment::load_from_disk() const {
    if (!events_.empty()) return;  // Already loaded

    // Load events
    auto events_path = dir_ / "events.dat";
    if (std::filesystem::exists(events_path)) {
        std::ifstream events_file(events_path, std::ios::binary | std::ios::ate);
        auto size = events_file.tellg();
        events_file.seekg(0);

        std::vector<uint8_t> compressed(size);
        events_file.read(reinterpret_cast<char*>(compressed.data()), size);

        auto decompressed = storage::Compression::decompress_to_string(
            compressed.data(), compressed.size()
        );

        auto events_json = nlohmann::json::parse(decompressed);
        events_.reserve(events_json.size());

        for (const auto& ej : events_json) {
            SegmentEvent event;
            event.id = ej["id"].get<uint64_t>();
            event.timestamp_us = ej["t"].get<uint64_t>();
            event.raw = ej["r"].get<std::string>();
            event.host = ej["h"].get<std::string>();
            event.source = ej["s"].get<std::string>();
            event.sourcetype = ej["st"].get<std::string>();
            events_.push_back(std::move(event));
        }
    }

    // Load inverted index
    auto index_path = dir_ / "index.idx";
    if (std::filesystem::exists(index_path)) {
        std::ifstream index_file(index_path, std::ios::binary | std::ios::ate);
        auto size = index_file.tellg();
        index_file.seekg(0);

        std::vector<uint8_t> data(size);
        index_file.read(reinterpret_cast<char*>(data.data()), size);

        inverted_index_ = std::make_unique<InvertedIndex>(
            InvertedIndex::deserialize(data.data(), data.size())
        );
    }

    // Load bloom filter
    auto bloom_path = dir_ / "bloom.bf";
    if (std::filesystem::exists(bloom_path)) {
        std::ifstream bloom_file(bloom_path, std::ios::binary | std::ios::ate);
        auto size = bloom_file.tellg();
        bloom_file.seekg(0);

        uint32_t num_hashes;
        bloom_file.read(reinterpret_cast<char*>(&num_hashes), sizeof(uint32_t));

        std::vector<uint8_t> data(size - sizeof(uint32_t));
        bloom_file.read(reinterpret_cast<char*>(data.data()), data.size());

        bloom_filter_ = std::make_unique<BloomFilter>(data, num_hashes);
    }
}

} // namespace sentinel::indexer
