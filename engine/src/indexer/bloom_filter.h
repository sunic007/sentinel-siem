#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>

namespace sentinel::indexer {

// Bloom filter for fast term existence checks on segments
class BloomFilter {
public:
    // Create with expected number of items and false positive rate
    BloomFilter(size_t expected_items, double false_positive_rate = 0.01);

    // Create from serialized data
    BloomFilter(const std::vector<uint8_t>& data, size_t num_hashes);

    // Add a term to the filter
    void add(std::string_view term);

    // Check if a term might be in the filter
    bool possibly_contains(std::string_view term) const;

    // Serialize for storage
    std::vector<uint8_t> serialize() const;

    // Stats
    size_t size_bytes() const { return bits_.size() / 8 + 1; }
    size_t num_hashes() const { return num_hashes_; }
    double fill_ratio() const;

private:
    std::vector<bool> bits_;
    size_t num_hashes_;

    // MurmurHash3-based hash functions
    std::pair<uint64_t, uint64_t> hash(std::string_view term) const;
    size_t nth_hash(size_t n, uint64_t hash1, uint64_t hash2) const;
};

} // namespace sentinel::indexer
