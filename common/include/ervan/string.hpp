#pragma once

#include <system_error>

#include <optional>
#include <tuple>
#include <unistd.h>

namespace ervan {
    enum parse_result {
        PARSE_OK,
        PARSE_INVALID,
        PARSE_END,
    };

    template <typename T>
    struct span {
        T*     start;
        size_t len;

        span() {
            this->start = nullptr;
            this->len   = 0;
        }

        span(T* _start, size_t _len) {
            this->start = _start;
            this->len   = _len;
        }

        span(T* _start, T* _end) {
            this->start = _start;
            this->len   = _end - _start;
        }

        T* begin() {
            return this->start;
        }

        T* end() {
            return this->start + this->len;
        }

        size_t size() {
            return this->len;
        }

        T& operator[](int index) {
            return this->start[index];
        }
    };

    struct join_result {
        span<char>       sp;
        span<const char> rest;
        std::errc        ec;

        join_result() {
            this->sp   = {};
            this->rest = {};
            this->ec   = {};
        }

        join_result(span<char> _sp, span<const char> _rest) {
            this->sp   = _sp;
            this->rest = _rest;
            this->ec   = {};
        }

        join_result(std::errc _ec, span<const char> _rest) {
            this->sp   = {};
            this->ec   = _ec;
            this->rest = _rest;
        }
    };

    std::tuple<const char*, const char*, const char*, const char*, size_t, parse_result>
    get_config_pair(const char* buffer, size_t len, size_t offset);

    join_result join(span<char> target, span<const char> src, int& src_offset,
                     span<const char> terminator);
}