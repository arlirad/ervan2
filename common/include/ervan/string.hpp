#pragma once

#include <system_error>

#include <optional>
#include <string>
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

    struct loop_result {
        span<const char> sp;
        span<const char> rest;
        std::errc        ec;
        bool             escape;

        loop_result() {
            this->sp     = {};
            this->rest   = {};
            this->ec     = {};
            this->escape = false;
        }

        loop_result(span<const char> _sp, span<const char> _rest, bool _escape = false) {
            this->sp     = _sp;
            this->rest   = _rest;
            this->ec     = {};
            this->escape = _escape;
        }

        loop_result(std::errc _ec, span<const char> _rest, bool _escape = false) {
            this->sp     = {};
            this->ec     = _ec;
            this->rest   = _rest;
            this->escape = _escape;
        }
    };

    struct loop_state {
        span<char>       t_check;
        size_t           max_size;
        int              total_length;
        size_t           max_line_length;
        int              line_length;
        span<const char> terminator;
        span<const char> escape;

        void reset() {
            this->total_length = 0;
            this->line_length  = 0;
        }

        size_t remaining_space() {
            if (this->total_length > this->max_size)
                return 0;

            return this->max_size - this->total_length;
        }
    };

    std::tuple<const char*, const char*, const char*, const char*, size_t, parse_result>
    get_config_pair(const char* buffer, size_t len, size_t offset);

    join_result join(span<char> target, span<const char> src, int& src_offset,
                     span<const char> terminator);
    loop_result look(loop_state& state, span<const char> _src);

    std::string get_unique_filename(const char* prefix);
}