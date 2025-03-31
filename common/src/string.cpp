#include "ervan/string.hpp"

#include <cctype>

namespace ervan {
    std::tuple<const char*, const char*, const char*, const char*, size_t, parse_result>
    get_config_entry(const char* buffer, size_t len, size_t offset) {
        while (isspace(buffer[offset]) && offset < len)
            offset++;

        if (offset == len)
            return {nullptr, nullptr, nullptr, nullptr, len, PARSE_END};

        const char* key_start = buffer + offset;

        while (!isspace(buffer[offset]) && offset < len)
            offset++;

        if (offset == len)
            return {nullptr, nullptr, nullptr, nullptr, len, PARSE_INVALID};

        const char* key_end = buffer + offset;

        while (isspace(buffer[offset]) && offset < len)
            offset++;

        if (offset == len)
            return {nullptr, nullptr, nullptr, nullptr, len, PARSE_INVALID};

        const char* value_start = buffer + offset;

        while (buffer[offset] != '\r' && buffer[offset] != '\n' && offset < len)
            offset++;

        const char* value_end = buffer + offset;

        return {key_start, key_end, value_start, value_end, offset, PARSE_OK};
    }
}