#pragma once

#include "indexer/segment.h"
#include <vector>
#include <memory>
#include <filesystem>

namespace sentinel::indexer {

// Merges small segments into larger ones
class Compaction {
public:
    struct Config {
        size_t min_segments_to_merge = 3;
        size_t max_segments_to_merge = 10;
        size_t target_segment_size_mb = 256;
    };

    explicit Compaction(Config config = {});

    // Select segments eligible for compaction
    std::vector<std::vector<Segment*>> plan(
        const std::vector<std::unique_ptr<Segment>>& segments
    ) const;

    // Merge multiple segments into one
    std::unique_ptr<Segment> merge(
        const std::vector<Segment*>& segments,
        const std::filesystem::path& output_dir,
        const std::string& index_name
    );

private:
    Config config_;
};

} // namespace sentinel::indexer
