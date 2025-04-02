#include "ervan/string.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>

using namespace ervan;

TEST(Base64Tests, Base64Test) {
    char buffer[32] = {};

    auto result = base64_encode({"Man", 3}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 4);
    ASSERT_EQ(std::strcmp(buffer, "TWFu"), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"Ma", 2}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 4);
    ASSERT_EQ(std::strcmp(buffer, "TWE="), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"M", 1}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 4);
    ASSERT_EQ(std::strcmp(buffer, "TQ=="), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"", static_cast<size_t>(0)}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 0);
    ASSERT_EQ(std::strcmp(buffer, ""), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"f", 1}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 4);
    ASSERT_EQ(std::strcmp(buffer, "Zg=="), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"fo", 2}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 4);
    ASSERT_EQ(std::strcmp(buffer, "Zm8="), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"foo", 3}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 4);
    ASSERT_EQ(std::strcmp(buffer, "Zm9v"), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"foob", 4}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 8);
    ASSERT_EQ(std::strcmp(buffer, "Zm9vYg=="), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"fooba", 5}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 8);
    ASSERT_EQ(std::strcmp(buffer, "Zm9vYmE="), 0);

    std::fill_n(buffer, sizeof(buffer), '\0');
    result = base64_encode({"foobar", 6}, {buffer, sizeof(buffer)});
    ASSERT_EQ(result, 8);
    ASSERT_EQ(std::strcmp(buffer, "Zm9vYmFy"), 0);
}