#include <gtest/gtest.h>
#include "ingest/timestamp_extractor.h"

using namespace sentinel::ingest;

TEST(TimestampExtractorTest, ISO8601) {
    auto ts = TimestampExtractor::extract("2024-01-15T10:30:00Z something happened");
    EXPECT_GT(ts, 0);
}

TEST(TimestampExtractorTest, EpochTimestamp) {
    auto ts = TimestampExtractor::extract("event at 1705312200.123 on server");
    EXPECT_GT(ts, 0);
}

TEST(TimestampExtractorTest, SyslogFormat) {
    auto ts = TimestampExtractor::extract("Jan 15 10:30:00 server01 sshd[1234]: Failed password");
    EXPECT_GT(ts, 0);
}

TEST(TimestampExtractorTest, NoTimestamp) {
    auto ts = TimestampExtractor::extract("just a plain message without time");
    EXPECT_EQ(ts, 0);
}
