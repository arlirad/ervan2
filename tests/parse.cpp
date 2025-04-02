#include "ervan/parse.hpp"

#include "ervan/string.hpp"
#include <gtest/gtest.h>

#include <cstring>

using namespace ervan;

TEST(ParseTests, ParsePathTest) {
    const char* input    = "<@relayA,@relayB.xyz,@relayC:giga.test.a@example.com> stray";
    span        input_sp = span<const char>(input, 59);

    auto result = parse::path(input_sp);

    ASSERT_EQ(std::strcmp(result.mailbox.local_part.buffer, "giga.test.a"), 0);
    ASSERT_EQ(std::strcmp(result.mailbox.domain.buffer, "example.com"), 0);
    ASSERT_EQ(result.rest.begin(), input + 53);
    ASSERT_EQ(result.rest.size(), 6);

    input    = "<a@b>";
    input_sp = span<const char>(input, 5);

    result = parse::path(input_sp);

    ASSERT_EQ(std::strcmp(result.mailbox.local_part.buffer, "a"), 0);
    ASSERT_EQ(std::strcmp(result.mailbox.domain.buffer, "b"), 0);
    ASSERT_EQ(result.rest.begin(), input + 5);
    ASSERT_EQ(result.rest.size(), 0);
}

TEST(ParseTests, ParseBufferSafetyTest) {
    const char* input =
        "<abcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefgh@example.com>";
    span input_sp = span<const char>(input, 78);

    auto result = parse::path(input_sp);

    ASSERT_EQ(std::strcmp(result.mailbox.local_part.buffer,
                          "abcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefgh"),
              0);
    ASSERT_EQ(std::strcmp(result.mailbox.domain.buffer, "example.com"), 0);
    ASSERT_EQ(result.rest.begin(), input + 78);
    ASSERT_EQ(result.rest.size(), 0);

    input    = "<abcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghA@example.com>";
    input_sp = span<const char>(input, 79);

    result = parse::path(input_sp);

    ASSERT_FALSE(result);
}

TEST(ParseTests, ParseQuotedStringTest) {
    const char* input    = "<\"asdf\\ bsdf\"@example.com>";
    span        input_sp = span<const char>(input, 26);

    auto result = parse::path(input_sp);

    ASSERT_EQ(std::strcmp(result.mailbox.local_part.buffer, "asdf bsdf"), 0);
    ASSERT_EQ(std::strcmp(result.mailbox.domain.buffer, "example.com"), 0);
    ASSERT_EQ(result.rest.begin(), input + 26);
    ASSERT_EQ(result.rest.size(), 0);

    input    = "<\"\"@example.com>";
    input_sp = span<const char>(input, 16);

    result = parse::path(input_sp);

    ASSERT_EQ(std::strcmp(result.mailbox.local_part.buffer, ""), 0);
    ASSERT_EQ(std::strcmp(result.mailbox.domain.buffer, "example.com"), 0);
    ASSERT_EQ(result.rest.begin(), input + 16);
    ASSERT_EQ(result.rest.size(), 0);
}

TEST(ParseTests, ParseESMTPParamTest) {
    const char* input    = "BODY=8BITMIME";
    span        input_sp = span<const char>(input, 13);

    auto result = parse::esmtp_param(input_sp);

    ASSERT_EQ(result.key.body, span<const char>("BODY", 4));
    ASSERT_EQ(result.value.body, span<const char>("8BITMIME", 4));
}