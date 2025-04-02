#include "ervan/string.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>

namespace ervan {
    char base64_alphabet[]{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
        'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
        'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '=',
    };

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

    size_t base64_encode(span<const char> src, span<char> dst) {
        size_t remaining  = src.size();
        size_t src_offset = 0;
        size_t dst_offset = 0;
        char   buffer[4];

        while (remaining >= 3) {
            char a = src[src_offset + 0];
            char b = src[src_offset + 1];
            char c = src[src_offset + 2];

            buffer[0] = base64_alphabet[((a & 0xFC) >> 2)];
            buffer[1] = base64_alphabet[((a & 0x03) << 4) | ((b & 0xF0) >> 4)];
            buffer[2] = base64_alphabet[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)];
            buffer[3] = base64_alphabet[((c & 0x3F) << 0)];

            std::copy_n(buffer, 4, dst.begin() + dst_offset);

            src_offset += 3;
            dst_offset += 4;
            remaining -= 3;
        }

        if (remaining >= 2) {
            char a = src[src_offset + 0];
            char b = src[src_offset + 1];

            buffer[0] = base64_alphabet[((a & 0xFC) >> 2)];
            buffer[1] = base64_alphabet[((a & 0x03) << 4) | ((b & 0xF0) >> 4)];
            buffer[2] = base64_alphabet[((b & 0x0F) << 2)];
            buffer[3] = base64_alphabet[64];

            std::copy_n(buffer, 4, dst.begin() + dst_offset);

            src_offset += 2;
            dst_offset += 4;
            remaining -= 2;
        }

        if (remaining >= 1) {
            char a = src[src_offset + 0];

            buffer[0] = base64_alphabet[((a & 0xFC) >> 2)];
            buffer[1] = base64_alphabet[((a & 0x03) << 4)];
            buffer[2] = base64_alphabet[64];
            buffer[3] = base64_alphabet[64];

            std::copy_n(buffer, 4, dst.begin() + dst_offset);

            src_offset += 1;
            dst_offset += 4;
            remaining -= 1;
        }

        return dst_offset;
    }

    size_t base64_decode(span<const char> src, span<char> dst) {
        return 0;
    }
}