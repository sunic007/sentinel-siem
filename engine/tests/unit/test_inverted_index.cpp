#include <gtest/gtest.h>
#include "indexer/inverted_index.h"

using namespace sentinel::indexer;

TEST(InvertedIndexTest, IndexAndSearch) {
    InvertedIndex idx;
    idx.index_event(0, "Failed login attempt from 192.168.1.1");
    idx.index_event(1, "Successful login from 192.168.1.2");
    idx.index_event(2, "Failed connection to database");

    auto results = idx.search("failed");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 0);
    EXPECT_EQ(results[1], 2);
}

TEST(InvertedIndexTest, AndSearch) {
    InvertedIndex idx;
    idx.index_event(0, "Failed login attempt from server");
    idx.index_event(1, "Successful login from server");
    idx.index_event(2, "Failed connection to database");

    auto results = idx.search_and({"failed", "login"});
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 0);
}

TEST(InvertedIndexTest, OrSearch) {
    InvertedIndex idx;
    idx.index_event(0, "error in module A");
    idx.index_event(1, "warning in module B");
    idx.index_event(2, "info message");

    auto results = idx.search_or({"error", "warning"});
    EXPECT_EQ(results.size(), 2);
}

TEST(InvertedIndexTest, Serialization) {
    InvertedIndex idx;
    idx.index_event(0, "test event data");
    idx.index_event(1, "another test event");

    auto data = idx.serialize();
    auto idx2 = InvertedIndex::deserialize(data.data(), data.size());

    EXPECT_EQ(idx.term_count(), idx2.term_count());
    EXPECT_FALSE(idx2.search("test").empty());
}
