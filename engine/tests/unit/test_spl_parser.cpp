#include <gtest/gtest.h>
#include "spl/parser.h"

using namespace sentinel::spl;

TEST(SPLParserTest, SimpleSearch) {
    Parser parser("search index=main error");
    auto query = parser.parse();

    EXPECT_FALSE(parser.has_errors());
    ASSERT_EQ(query->commands.size(), 1);
    EXPECT_EQ(query->commands[0]->type, CommandType::SEARCH);

    auto* search = static_cast<SearchNode*>(query->commands[0].get());
    EXPECT_EQ(search->index, "main");
    EXPECT_FALSE(search->search_terms.empty());
}

TEST(SPLParserTest, PipedCommands) {
    Parser parser("search index=main | stats count by host | sort -count | head 10");
    auto query = parser.parse();

    EXPECT_FALSE(parser.has_errors());
    EXPECT_EQ(query->commands.size(), 4);
    EXPECT_EQ(query->commands[0]->type, CommandType::SEARCH);
    EXPECT_EQ(query->commands[1]->type, CommandType::STATS);
    EXPECT_EQ(query->commands[2]->type, CommandType::SORT);
    EXPECT_EQ(query->commands[3]->type, CommandType::HEAD);
}

TEST(SPLParserTest, StatsWithGroupBy) {
    Parser parser("stats count, avg(duration) by host, sourcetype");
    auto query = parser.parse();

    EXPECT_FALSE(parser.has_errors());
    auto* stats = static_cast<StatsNode*>(query->commands[0].get());
    EXPECT_EQ(stats->aggregations.size(), 2);
    EXPECT_EQ(stats->group_by.size(), 2);
}

TEST(SPLParserTest, Table) {
    Parser parser("table host, source, _raw");
    auto query = parser.parse();

    EXPECT_FALSE(parser.has_errors());
    auto* table = static_cast<TableNode*>(query->commands[0].get());
    EXPECT_EQ(table->fields.size(), 3);
}

TEST(SPLParserTest, Rename) {
    Parser parser("rename old_field AS new_field, src AS source_ip");
    auto query = parser.parse();

    EXPECT_FALSE(parser.has_errors());
    auto* rename = static_cast<RenameNode*>(query->commands[0].get());
    EXPECT_EQ(rename->mappings.size(), 2);
    EXPECT_EQ(rename->mappings[0].first, "old_field");
    EXPECT_EQ(rename->mappings[0].second, "new_field");
}
