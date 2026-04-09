#include "indexer/bloom_filter.h"
#include <cmath>
#include <algorithm>

namespace sentinel::indexer {

BloomFilter::BloomFilter(size_t expected_items, double false_positive_rate) {
    // Calculate optimal size and number of hashes
    double ln2 = std::log(2.0);
    size_t num_bits = static_cast<size_t>(
        -static_cast<double>(expected_items) * std::log(false_positive_rate) / (ln2 * ln2)
    );
    num_bits = std::max(num_bits, size_t(64));

    num_hashes_ = static_cast<size_t>(
        static_cast<double>(num_bits) / static_cast<double>(expected_items) * ln2
    );
    num_hashes_ = std::max(num_hashes_, size_t(1));
    num_hashes_ = std::min(num_hashes_, size_t(16));

    bits_.resize(num_bits, false);
}

BloomFilter::BloomFilter(const std::vector<uint8_t>& data, size_t num_hashes)
    : num_hashes_(num_hashes) {
    bits_.resize(data.size() * 8);
    for (size_t i = 0; i < data.size(); ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            bits_[i * 8 + bit] = (data[i] >> bit) & 1;
        }
    }
}

void BloomFilter::add(std::string_view term) {
    auto [h1, h2] = hash(term);
    for (size_t i = 0; i < num_hashes_; ++i) {
        bits_[nth_hash(i, h1, h2)] = true;
    }
}

bool BloomFilter::possibly_contains(std::string_view term) const {
    auto [h1, h2] = hash(term);
    for (size_t i = 0; i < num_hashes_; ++i) {
        if (!bits_[nth_hash(i, h1, h2)]) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> BloomFilter::serialize() const {
    size_t byte_count = (bits_.size() + 7) / 8;
    std::vector<uint8_t> data(byte_count, 0);

    for (size_t i = 0; i < bits_.size(); ++i) {
        if (bits_[i]) {
            data[i / 8] |= (1 << (i % 8));
        }
    }

    return data;
}

double BloomFilter::fill_ratio() const {
    size_t set_bits = 0;
    for (bool b : bits_) {
        if (b) ++set_bits;
    }
    return static_cast<double>(set_bits) / static_cast<double>(bits_.size());
}

std::pair<uint64_t, uint64_t> BloomFilter::hash(std::string_view term) const {
    // Simple hash using FNV-1a variant for two independent hashes
    uint64_t h1 = 14695981039346656037ULL;
    uint64_t h2 = 0xcbf29ce484222325ULL;

    for (char c : term) {
        h1 ^= static_cast<uint64_t>(c);
        h1 *= 1099511628211ULL;

        h2 ^= static_cast<uint64_t>(c);
        h2 *= 6364136223846793005ULL;
        h2 += 1442695040888963407ULL;
    }

    return {h1, h2};
}

size_t BloomFilter::nth_hash(size_t n, uint64_t hash1, uint64_t hash2) const {
    return (hash1 + n * hash2) % bits_.size();
}

} // namespace sentinel::indexer
