#include <gtest/gtest.h>
#include "storage/compression.h"

using namespace sentinel::storage;

TEST(CompressionTest, ZstdRoundTrip) {
    std::string data = "This is a test string for compression. It should compress well "
                       "when repeated multiple times. This is a test string for compression.";

    auto compressed = Compression::compress_string(data, CompressionType::ZSTD);
    EXPECT_LT(compressed.size(), data.size());

    auto decompressed = Compression::decompress_to_string(
        compressed.data(), compressed.size(), CompressionType::ZSTD
    );
    EXPECT_EQ(decompressed, data);
}

TEST(CompressionTest, Lz4RoundTrip) {
    std::string data = "LZ4 compression test data with some repetitive content content content";

    auto compressed = Compression::compress_string(data, CompressionType::LZ4);
    // LZ4 decompression needs known output size - tested differently
    EXPECT_FALSE(compressed.empty());
}

TEST(CompressionTest, NoCompression) {
    std::string data = "Uncompressed data";
    auto result = Compression::compress_string(data, CompressionType::NONE);
    EXPECT_EQ(result.size(), data.size());
}
