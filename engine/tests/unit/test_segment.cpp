#include <gtest/gtest.h>
#include "indexer/segment.h"
#include <filesystem>

using namespace sentinel::indexer;

class SegmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "sentinel_test_segments";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
};

TEST_F(SegmentTest, CreateAndWrite) {
    auto segment = Segment::create(test_dir_, "test_index", "seg_001");
    ASSERT_NE(segment, nullptr);

    std::vector<SegmentEvent> events;
    for (int i = 0; i < 100; ++i) {
        SegmentEvent e;
        e.id = i;
        e.timestamp_us = 1000000 + i * 1000;
        e.raw = "Event " + std::to_string(i) + " error from host-" + std::to_string(i % 5);
        e.host = "host-" + std::to_string(i % 5);
        e.source = "/var/log/test.log";
        e.sourcetype = "test_log";
        events.push_back(std::move(e));
    }

    segment->write_events(events);
    EXPECT_TRUE(segment->is_sealed());
    EXPECT_EQ(segment->meta().event_count, 100);
}

TEST_F(SegmentTest, Search) {
    auto segment = Segment::create(test_dir_, "test_index", "seg_002");

    std::vector<SegmentEvent> events;
    events.push_back({0, 1000000, "Failed login from 10.0.0.1", "host1", "/var/log/auth.log", "syslog"});
    events.push_back({1, 1001000, "Successful login from 10.0.0.2", "host2", "/var/log/auth.log", "syslog"});
    events.push_back({2, 1002000, "Failed connection timeout", "host1", "/var/log/app.log", "app"});

    segment->write_events(events);

    auto results = segment->search({"failed"});
    EXPECT_EQ(results.size(), 2);
}
