#include <gtest/gtest.h>
#include "spl/lexer.h"

using namespace sentinel::spl;

TEST(SPLLexerTest, SimpleSearch) {
    Lexer lexer("search index=main error");
    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::KW_SEARCH);
    EXPECT_EQ(tokens[1].type, TokenType::KW_INDEX);
    EXPECT_EQ(tokens[2].type, TokenType::EQUALS);
    EXPECT_EQ(tokens[3].value, "main");
}

TEST(SPLLexerTest, PipedCommands) {
    Lexer lexer("search error | stats count by host | sort -count | head 10");
    auto tokens = lexer.tokenize();

    int pipes = 0;
    for (const auto& t : tokens) {
        if (t.type == TokenType::PIPE) ++pipes;
    }
    EXPECT_EQ(pipes, 3);
}

TEST(SPLLexerTest, StringLiterals) {
    Lexer lexer(R"(search host="web-server-01")");
    auto tokens = lexer.tokenize();

    bool found_string = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::STRING && t.value == "web-server-01") {
            found_string = true;
        }
    }
    EXPECT_TRUE(found_string);
}

TEST(SPLLexerTest, Aggregations) {
    Lexer lexer("stats count, avg(duration), max(bytes) by sourcetype");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, TokenType::KW_STATS);
    EXPECT_EQ(tokens[1].type, TokenType::KW_COUNT);
}
