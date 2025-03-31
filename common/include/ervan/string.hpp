#pragma once

#include <optional>
#include <tuple>
#include <unistd.h>

namespace ervan {
    enum parse_result {
        PARSE_OK,
        PARSE_INVALID,
        PARSE_END,
    };

    std::tuple<const char*, const char*, const char*, const char*, size_t, parse_result>
    get_config_entry(const char* buffer, size_t len, size_t offset);
}