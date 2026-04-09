#include <gtest/gtest.h>
#include "indexer/bloom_filter.h"

using namespace sentinel::indexer;

TEST(BloomFilterTest, BasicAddAndContains) {
    BloomFilter bf(1000, 0.01);

    bf.add("hello");
    bf.add("world");
    bf.add("error");

    EXPECT_TRUE(bf.possibly_contains("hello"));
    EXPECT_TRUE(bf.possibly_contains("world"));
    EXPECT_TRUE(bf.possibly_contains("error"));
}

TEST(BloomFilterTest, NegativeLookup) {
    BloomFilter bf(1000, 0.01);

    bf.add("hello");

    // Should have very low false positive rate
    // Not a deterministic test, but with 0.01 FP rate it should pass
    int false_positives = 0;
    for (int i = 0; i < 100; ++i) {
        if (bf.possibly_contains("notexist_" + std::to_string(i))) {
            ++false_positives;
        }
    }
    EXPECT_LT(false_positives, 10);  // Allow some false positives
}

TEST(BloomFilterTest, Serialization) {
    BloomFilter bf(100, 0.01);
    bf.add("test1");
    bf.add("test2");

    auto data = bf.serialize();
    BloomFilter bf2(data, bf.num_hashes());

    EXPECT_TRUE(bf2.possibly_contains("test1"));
    EXPECT_TRUE(bf2.possibly_contains("test2"));
}
