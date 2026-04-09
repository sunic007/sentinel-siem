#pragma once

#include <vector>
#include <cstdint>
#include <string_view>

namespace sentinel::storage {

enum class CompressionType {
    NONE,
    ZSTD,
    LZ4
};

class Compression {
public:
    // Compress data using the specified algorithm
    static std::vector<uint8_t> compress(
        const void* data, size_t size,
        CompressionType type = CompressionType::ZSTD,
        int level = 3
    );

    // Decompress data
    static std::vector<uint8_t> decompress(
        const void* data, size_t size,
        CompressionType type = CompressionType::ZSTD
    );

    // Compress a string
    static std::vector<uint8_t> compress_string(
        std::string_view input,
        CompressionType type = CompressionType::ZSTD,
        int level = 3
    );

    // Decompress to string
    static std::string decompress_to_string(
        const void* data, size_t size,
        CompressionType type = CompressionType::ZSTD
    );

    // Get compression ratio estimate
    static double estimate_ratio(CompressionType type);
};

} // namespace sentinel::storage
