#pragma once

#include <system_error>

#include <cstring>
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

    extern char base64_alphabet[65];

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

        constexpr T* begin() const {
            return this->start;
        }

        constexpr T* end() const {
            return this->start + this->len;
        }

        constexpr size_t size() const {
            return this->len;
        }

        constexpr T& operator[](int index) const {
            return this->start[index];
        }

        constexpr span advance(size_t n) const {
            return span(this->begin() + n, this->end());
        }

        bool compare_insensitive(const span& other) const {
            if (this->size() != other.size())
                return false;

            for (size_t i = 0; i < this->size(); i++)
                if (std::tolower(this->start[i]) != std::tolower(other.start[i]))
                    return false;

            return true;
        }

        bool operator==(const span& other) const {
            return this->size() != other.size()
                       ? false
                       : std::memcmp(this->begin(), other.begin(), this->size()) == 0;
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

        size_t remaining_space() const {
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

    size_t base64_encode(span<const char> src, span<char> dst);
    size_t base64_decode(span<const char> src, span<char> dst);
}