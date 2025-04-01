#include "ervan/string.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>

namespace ervan {
    std::tuple<const char*, const char*, const char*, const char*, size_t, parse_result>
    get_config_pair(const char* buffer, size_t len, size_t offset) {
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

    void push_back(span<char> buffer, char c) {
        for (int i = 0; i < buffer.size() - 1; i++)
            buffer[i] = buffer[i + 1];

        buffer[buffer.size() - 1] = c;
    }

    join_result join(span<char> _target, span<const char> _src, int& tgt_offset,
                     span<const char> terminator) {
        if (_src.size() == 0)
            return {};

        auto target  = span(_target.begin(), _target.len - terminator.len);
        auto src     = span(_src.begin(), std::min(target.len, _src.len));
        auto t_check = span(target.end(), _target.end());

        size_t cpy_len = std::min(_target.len - tgt_offset, src.len);

        // std::copy doesn't like mixing const with non const
        std::memcpy(target.begin() + tgt_offset, src.begin(), cpy_len);

        for (int i = 0; i < _src.size(); i++) {
            push_back(t_check, _src[i]);

            if (memcmp(t_check.begin(), terminator.begin(), t_check.size()) == 0) {
                size_t target_len = tgt_offset - t_check.size() + 1;
                tgt_offset        = 0;

                if (target_len > target.size())
                    return {std::errc::result_out_of_range, {_src.begin() + i + 1, _src.end()}};

                *(target.begin() + target_len) = '\0';

                return {{target.begin(), target_len}, {_src.begin() + i + 1, _src.end()}};
            }

            tgt_offset++;
        }

        return {{}, {_src.end(), _src.end()}};
    }

    // TODO: this needs to be rewritten to be more generic
    loop_result look(loop_state& state, span<const char> _src) {
        if (_src.size() == 0)
            return {};

        for (int i = 0; i < _src.size(); i++) {
            push_back(state.t_check, _src[i]);

            if (memcmp(state.t_check.end() - state.escape.size(), state.escape.begin(),
                       state.escape.size()) == 0) {
                state.total_length += i + 1;

                return {{_src.begin(), _src.begin() + i}, {_src.begin() + i + 1, _src.end()}, true};
            }

            if (memcmp(state.t_check.begin(), state.terminator.begin(), state.t_check.size()) ==
                0) {
                size_t target_len  = (state.total_length + i) - state.t_check.size() + 1;
                state.total_length = 0;

                if (target_len > state.max_size)
                    return {std::errc::result_out_of_range, {_src.begin() + i + 1, _src.end()}};

                return {{state.t_check.begin(), target_len}, {_src.begin() + i + 1, _src.end()}};
            }
        }

        state.total_length += _src.size();

        return {{}, {_src.end(), _src.end()}};
    }

    std::string get_unique_filename(const char* prefix) {
        static int counter = 0;
        return std::format("{}.{}.{}.{}.{}", prefix, getpid(), std::time(nullptr), counter++,
                           std::rand());
    }
}