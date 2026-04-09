#include "storage/compression.h"
#include <zstd.h>
#include <lz4.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace sentinel::storage {

std::vector<uint8_t> Compression::compress(
    const void* data, size_t size,
    CompressionType type, int level
) {
    if (type == CompressionType::NONE) {
        auto ptr = static_cast<const uint8_t*>(data);
        return {ptr, ptr + size};
    }

    if (type == CompressionType::ZSTD) {
        size_t bound = ZSTD_compressBound(size);
        std::vector<uint8_t> output(bound);

        size_t compressed_size = ZSTD_compress(
            output.data(), bound,
            data, size,
            level
        );

        if (ZSTD_isError(compressed_size)) {
            throw std::runtime_error(
                std::string("ZSTD compression failed: ") + ZSTD_getErrorName(compressed_size)
            );
        }

        output.resize(compressed_size);
        return output;
    }

    if (type == CompressionType::LZ4) {
        int bound = LZ4_compressBound(static_cast<int>(size));
        std::vector<uint8_t> output(bound);

        int compressed_size = LZ4_compress_default(
            static_cast<const char*>(data),
            reinterpret_cast<char*>(output.data()),
            static_cast<int>(size),
            bound
        );

        if (compressed_size <= 0) {
            throw std::runtime_error("LZ4 compression failed");
        }

        output.resize(compressed_size);
        return output;
    }

    throw std::runtime_error("Unknown compression type");
}

std::vector<uint8_t> Compression::decompress(
    const void* data, size_t size,
    CompressionType type
) {
    if (type == CompressionType::NONE) {
        auto ptr = static_cast<const uint8_t*>(data);
        return {ptr, ptr + size};
    }

    if (type == CompressionType::ZSTD) {
        size_t decompressed_size = ZSTD_getFrameContentSize(data, size);
        if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN ||
            decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
            throw std::runtime_error("Cannot determine decompressed size");
        }

        std::vector<uint8_t> output(decompressed_size);
        size_t result = ZSTD_decompress(
            output.data(), decompressed_size,
            data, size
        );

        if (ZSTD_isError(result)) {
            throw std::runtime_error(
                std::string("ZSTD decompression failed: ") + ZSTD_getErrorName(result)
            );
        }

        output.resize(result);
        return output;
    }

    throw std::runtime_error("LZ4 decompression requires known output size");
}

std::vector<uint8_t> Compression::compress_string(
    std::string_view input, CompressionType type, int level
) {
    return compress(input.data(), input.size(), type, level);
}

std::string Compression::decompress_to_string(
    const void* data, size_t size, CompressionType type
) {
    auto decompressed = decompress(data, size, type);
    return {reinterpret_cast<const char*>(decompressed.data()), decompressed.size()};
}

double Compression::estimate_ratio(CompressionType type) {
    switch (type) {
        case CompressionType::ZSTD: return 0.25;  // ~4x compression for logs
        case CompressionType::LZ4:  return 0.40;  // ~2.5x compression
        case CompressionType::NONE: return 1.0;
    }
    return 1.0;
}

} // namespace sentinel::storage
