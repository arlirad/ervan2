#include "ervan/string.hpp"
#include <gtest/gtest.h>

#include <cstring>

const char terminator[2] = {'\r', '\n'};
auto       term_sp       = ervan::span(terminator, 2);

const char terminator_long[5] = {'\r', '\n', '.', '\r', '\n'};
auto       term_l_sp          = ervan::span(terminator_long, 5);

const char escape[4] = {'\r', '\n', '.', '.'};
auto       escape_sp = ervan::span(escape, 4);

TEST(BufferTests, BufferJoinContinuousTest) {
    char buffer[8];
    int  offset = 0;

    auto part = "aa\r\nbb";

    auto result = join(ervan::span(buffer, sizeof(buffer)), ervan::span(part, 6), offset, term_sp);

    ASSERT_EQ(result.sp.begin(), buffer);
    ASSERT_EQ(result.sp.size(), 2);
    ASSERT_EQ(result.rest.begin(), part + 4);
    ASSERT_EQ(result.rest.size(), 2);
    ASSERT_TRUE(std::strcmp(buffer, "aa") == 0);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferJoinChainedTest) {
    char buffer[8];
    int  offset = 0;

    auto chain = "aa\r\nbb\r\n";

    auto result = join(ervan::span(buffer, sizeof(buffer)), ervan::span(chain, 8), offset, term_sp);

    ASSERT_EQ(result.sp.begin(), buffer);
    ASSERT_EQ(result.sp.size(), 2);
    ASSERT_EQ(result.rest.begin(), chain + 4);
    ASSERT_EQ(result.rest.size(), 4);
    ASSERT_TRUE(std::strcmp(buffer, "aa") == 0);
    ASSERT_EQ(offset, 0);

    result = join(ervan::span(buffer, sizeof(buffer)), result.rest, offset, term_sp);

    ASSERT_EQ(result.sp.begin(), buffer);
    ASSERT_EQ(result.sp.size(), 2);
    ASSERT_EQ(result.rest.begin(), chain + 8);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_TRUE(std::strcmp(buffer, "bb") == 0);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferJoinPiecedTest) {
    char buffer[8];
    int  offset = 0;

    auto part = "aa";

    auto result = join(ervan::span(buffer, sizeof(buffer)), ervan::span(part, 2), offset, term_sp);

    ASSERT_EQ(result.sp.begin(), nullptr);
    ASSERT_EQ(result.sp.size(), 0);
    ASSERT_EQ(result.rest.begin(), part + 2);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_EQ(offset, 2);

    auto part2 = "\r\n";

    auto result2 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part2, 2), offset, term_sp);

    ASSERT_EQ(result2.sp.begin(), buffer);
    ASSERT_EQ(result2.sp.size(), 2);
    ASSERT_EQ(result2.rest.begin(), part2 + 2);
    ASSERT_EQ(result2.rest.size(), 0);
    ASSERT_TRUE(std::strcmp(buffer, "aa") == 0);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferJoinChainedPiecedTest) {
    char buffer[16];
    int  offset = 0;

    auto chain  = "ab";
    auto chain2 = "cd\r\nff";
    auto result = join(ervan::span(buffer, sizeof(buffer)), ervan::span(chain, 2), offset, term_sp);

    ASSERT_EQ(result.sp.begin(), nullptr);
    ASSERT_EQ(result.sp.size(), 0);
    ASSERT_EQ(result.rest.begin(), chain + 2);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_EQ(offset, 2);

    auto result2 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(chain2, 6), offset, term_sp);

    ASSERT_EQ(result2.sp.begin(), buffer);
    ASSERT_EQ(result2.sp.size(), 4);
    ASSERT_EQ(result2.rest.begin(), chain2 + 4);
    ASSERT_EQ(result2.rest.size(), 2);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferJoinMaxLengthTest) {
    char buffer[8];
    char guard[32] = {};
    int  offset    = 0;

    auto part   = "abcdef\r\n";
    auto result = join(ervan::span(buffer, sizeof(buffer)), ervan::span(part, 8), offset, term_sp);

    ASSERT_EQ(result.sp.begin(), buffer);
    ASSERT_EQ(result.sp.size(), 6);
    ASSERT_EQ(result.rest.begin(), part + 8);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_NE(result.ec, std::errc::result_out_of_range);
    ASSERT_EQ(offset, 0);

    auto part2 = "abcd";
    auto result2 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part2, 4), offset, term_sp);

    ASSERT_EQ(result2.sp.begin(), nullptr);
    ASSERT_EQ(result2.sp.size(), 0);
    ASSERT_EQ(result2.rest.begin(), part2 + 4);
    ASSERT_EQ(result2.rest.size(), 0);
    ASSERT_NE(result2.ec, std::errc::result_out_of_range);
    ASSERT_EQ(offset, 4);

    auto part3 = "efhijklmno";
    auto result3 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part3, 10), offset, term_sp);

    ASSERT_EQ(result3.sp.begin(), nullptr);
    ASSERT_EQ(result3.sp.size(), 0);
    ASSERT_EQ(result3.rest.begin(), part3 + 10);
    ASSERT_EQ(result3.rest.size(), 0);
    ASSERT_NE(result3.ec, std::errc::result_out_of_range);
    ASSERT_EQ(offset, 14);
    ASSERT_EQ(guard[0], '\0');

    auto part4 = "\r\n";
    auto result4 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part4, 2), offset, term_sp);

    ASSERT_EQ(result4.sp.begin(), nullptr);
    ASSERT_EQ(result4.sp.size(), 0);
    ASSERT_EQ(result4.rest.begin(), part4 + 2);
    ASSERT_EQ(result4.rest.size(), 0);
    ASSERT_EQ(result4.ec, std::errc::result_out_of_range);
    ASSERT_EQ(offset, 0);
    ASSERT_EQ(guard[0], '\0');
}

TEST(BufferTests, BufferJoinBrokenTerminatorTest) {
    char buffer[16];
    int  offset = 0;

    auto part  = "aaa\r";
    auto part2 = "\n";

    auto result = join(ervan::span(buffer, sizeof(buffer)), ervan::span(part, 4), offset, term_sp);

    ASSERT_EQ(result.sp.begin(), nullptr);
    ASSERT_EQ(result.sp.size(), 0);
    ASSERT_EQ(result.rest.begin(), part + 4);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_EQ(offset, 4);

    auto result2 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part2, 1), offset, term_sp);

    ASSERT_EQ(result2.sp.begin(), buffer);
    ASSERT_EQ(result2.sp.size(), 3);
    ASSERT_EQ(result2.rest.begin(), part2 + 1);
    ASSERT_EQ(result2.rest.size(), 0);
    ASSERT_TRUE(std::strcmp(buffer, "aaa") == 0);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferJoinLongBrokenTerminatorTest) {
    char buffer[16];
    int  offset = 0;

    auto part  = "aaa\r";
    auto part2 = "\n.";
    auto part3 = "\r\n";

    auto result =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part, 4), offset, term_l_sp);

    ASSERT_EQ(result.sp.begin(), nullptr);
    ASSERT_EQ(result.sp.size(), 0);
    ASSERT_EQ(result.rest.begin(), part + 4);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_EQ(offset, 4);

    auto result2 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part2, 2), offset, term_l_sp);

    ASSERT_EQ(result2.sp.begin(), nullptr);
    ASSERT_EQ(result2.sp.size(), 0);
    ASSERT_EQ(result2.rest.begin(), part2 + 2);
    ASSERT_EQ(result2.rest.size(), 0);
    ASSERT_EQ(offset, 6);

    auto result3 =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part3, 2), offset, term_l_sp);

    ASSERT_EQ(result3.sp.begin(), buffer);
    ASSERT_EQ(result3.sp.size(), 3);
    ASSERT_EQ(result3.rest.begin(), part3 + 2);
    ASSERT_EQ(result3.rest.size(), 0);
    ASSERT_TRUE(std::strcmp(buffer, "aaa") == 0);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferJoinLongTerminatorTest) {
    char buffer[16];
    int  offset = 0;

    auto part = "aaa\r\n.\r\nbbbb";

    auto result =
        join(ervan::span(buffer, sizeof(buffer)), ervan::span(part, 12), offset, term_l_sp);

    ASSERT_EQ(result.sp.begin(), buffer);
    ASSERT_EQ(result.sp.size(), 3);
    ASSERT_EQ(result.rest.begin(), part + 8);
    ASSERT_EQ(result.rest.size(), 4);
    ASSERT_TRUE(std::strcmp(buffer, "aaa") == 0);
    ASSERT_EQ(offset, 0);
}

TEST(BufferTests, BufferLookEscapeTest) {
    char buffer[5];

    auto part  = "aaa\r\n..\r\nbbbb\r\n.\r\n";
    auto state = ervan::loop_state{
        .t_check         = ervan::span(buffer, sizeof(buffer)),
        .max_size        = 32,
        .max_line_length = 100,
        .terminator      = term_l_sp,
        .escape          = escape_sp,
    };

    auto result = look(state, ervan::span(part, 18));

    ASSERT_TRUE(result.escape);
    ASSERT_EQ(result.sp.begin(), part);
    ASSERT_EQ(result.sp.size(), 6);
    ASSERT_EQ(result.rest.begin(), part + 7);
    ASSERT_EQ(result.rest.size(), 11);
    ASSERT_EQ(state.total_length, 7);
}

TEST(BufferTests, BufferLookBrokenEscapeTest) {
    char buffer[5];

    auto part  = "aaa\r\n.";
    auto state = ervan::loop_state{
        .t_check         = ervan::span(buffer, sizeof(buffer)),
        .max_size        = 32,
        .max_line_length = 100,
        .terminator      = term_l_sp,
        .escape          = escape_sp,
    };

    auto result = look(state, ervan::span(part, 6));

    ASSERT_FALSE(result.escape);
    ASSERT_EQ(result.sp.begin(), nullptr);
    ASSERT_EQ(result.sp.size(), 0);
    ASSERT_EQ(result.rest.begin(), part + 6);
    ASSERT_EQ(result.rest.size(), 0);
    ASSERT_EQ(state.total_length, 6);

    auto part2   = ".\r\nbbbb\r\n.\r\n";
    auto result2 = look(state, ervan::span(part2, 12));

    ASSERT_EQ(result2.sp.begin(), part2);
    ASSERT_EQ(result2.sp.size(), 0);
    ASSERT_EQ(result2.rest.begin(), part2 + 1);
    ASSERT_EQ(result2.rest.size(), 11);
}