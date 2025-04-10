#include "ervan/rfc5322.hpp"

#include <cstring>

namespace ervan::rfc5322 {
    writer::writer(eaio::file& file) : base(file) {}

    eaio::coro<bool> writer::begin_header(const char* name) {
        size_t name_len = std::strlen(name);

        this->line_length += name_len;

        if (!co_await this->base.write(name, name_len))
            co_return false;

        co_return co_await this->base.write(":", 1);
    }

    eaio::coro<bool> writer::write_body_part(const char* part, bool space) {
        size_t part_len = std::strlen(part);

        if (this->line_length + (space ? 1 : 0) + part_len > preffered_line_length) {
            if (!co_await this->base.write("\r\n\t", 3))
                co_return false;

            this->line_length = 1;
        }
        else if (space) {
            if (!co_await this->base.write(" ", 1))
                co_return false;

            this->line_length++;
        }

        this->line_length += part_len;

        if (!co_await this->base.write(part, part_len))
            co_return false;

        co_return true;
    }

    eaio::coro<bool> writer::write_time(tm* t) {
        auto write = [this, t](const char* fmt, bool space = true) -> eaio::coro<bool> {
            char datetime_buffer[32] = {};
            std::strftime(datetime_buffer, sizeof(datetime_buffer) - 1, fmt, t);

            co_return co_await this->write_body_part(datetime_buffer, space);
        };

        if (!co_await write("%a,"))
            co_return false;

        if (!co_await write("%d"))
            co_return false;

        if (!co_await write("%b"))
            co_return false;

        if (!co_await write("%Y"))
            co_return false;

        if (!co_await write("%T"))
            co_return false;

        if (!co_await write("%z"))
            co_return false;

        co_return true;
    }

    eaio::coro<bool> writer::end_header() {
        co_return co_await this->base.write("\r\n", 2);
    }
}