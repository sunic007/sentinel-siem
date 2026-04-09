#include "indexer/compaction.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>

namespace sentinel::indexer {

Compaction::Compaction(Config config) : config_(std::move(config)) {}

std::vector<std::vector<Segment*>> Compaction::plan(
    const std::vector<std::unique_ptr<Segment>>& segments
) const {
    std::vector<std::vector<Segment*>> merge_groups;

    // Collect small sealed segments
    std::vector<Segment*> candidates;
    size_t target_bytes = config_.target_segment_size_mb * 1024 * 1024;

    for (const auto& seg : segments) {
        if (seg->is_sealed() && seg->meta().size_bytes < target_bytes / 2) {
            candidates.push_back(seg.get());
        }
    }

    // Sort by time range for temporal locality
    std::sort(candidates.begin(), candidates.end(),
              [](const Segment* a, const Segment* b) {
                  return a->meta().earliest_time < b->meta().earliest_time;
              });

    // Group adjacent segments up to max merge count
    for (size_t i = 0; i < candidates.size();) {
        size_t group_size = std::min(config_.max_segments_to_merge,
                                     candidates.size() - i);
        if (group_size >= config_.min_segments_to_merge) {
            std::vector<Segment*> group(
                candidates.begin() + i,
                candidates.begin() + i + group_size
            );
            merge_groups.push_back(std::move(group));
            i += group_size;
        } else {
            break;
        }
    }

    return merge_groups;
}

std::unique_ptr<Segment> Compaction::merge(
    const std::vector<Segment*>& segments,
    const std::filesystem::path& output_dir,
    const std::string& index_name
) {
    if (segments.empty()) return nullptr;

    // Collect all events from source segments
    std::vector<SegmentEvent> all_events;
    for (const auto* seg : segments) {
        auto events = seg->scan();
        all_events.insert(all_events.end(),
                         std::make_move_iterator(events.begin()),
                         std::make_move_iterator(events.end()));
    }

    // Sort by timestamp
    std::sort(all_events.begin(), all_events.end(),
              [](const auto& a, const auto& b) {
                  return a.timestamp_us < b.timestamp_us;
              });

    // Reassign IDs
    for (size_t i = 0; i < all_events.size(); ++i) {
        all_events[i].id = i;
    }

    // Create new merged segment
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::string segment_id = "seg_merged_" + std::to_string(ms);

    auto merged = Segment::create(output_dir, index_name, segment_id);
    merged->write_events(all_events);

    spdlog::info("Compaction: merged {} segments ({} events) into {}",
                 segments.size(), all_events.size(), segment_id);

    return merged;
}

} // namespace sentinel::indexer
