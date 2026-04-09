#include <gtest/gtest.h>
#include "indexer/wal.h"
#include <filesystem>

using namespace sentinel::indexer;

class WalTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "sentinel_test_wal";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
};

TEST_F(WalTest, AppendAndRead) {
    Wal wal(test_dir_);

    WalEntry entry;
    entry.timestamp_us = 1000000;
    entry.index_name = "main";
    entry.raw_event = "Test event 1";
    entry.host = "host1";
    entry.source = "/var/log/test.log";
    entry.sourcetype = "syslog";

    auto seq = wal.append(entry);
    EXPECT_GT(seq, 0);

    auto entries = wal.read_all();
    EXPECT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].raw_event, "Test event 1");
}

TEST_F(WalTest, MultipleEntries) {
    Wal wal(test_dir_);

    for (int i = 0; i < 100; ++i) {
        WalEntry entry;
        entry.timestamp_us = 1000000 + i;
        entry.index_name = "main";
        entry.raw_event = "Event " + std::to_string(i);
        entry.host = "host";
        entry.source = "test";
        entry.sourcetype = "test";
        wal.append(entry);
    }

    auto entries = wal.read_all();
    EXPECT_EQ(entries.size(), 100);
}
