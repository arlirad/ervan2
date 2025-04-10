#include "ervan/message.hpp"

#include "ervan/io.hpp"
#include "ervan/string.hpp"

namespace ervan {
    eaio::coro<bool> metadata::write_to(eaio::file& file) {
        const size_t line_len    = sizeof(this->line_a);
        const size_t line_len_64 = (((line_len + (line_len) / 3) + 3) / 4) * 4;

        char buffer[line_len_64 * 4 + 8];

        base64_encode({this->line_a, line_len}, {buffer + (line_len_64 + 2) * 0, line_len_64});
        base64_encode({this->line_b, line_len}, {buffer + (line_len_64 + 2) * 1, line_len_64});
        base64_encode({this->line_c, line_len}, {buffer + (line_len_64 + 2) * 2, line_len_64});
        base64_encode({this->line_d, line_len}, {buffer + (line_len_64 + 2) * 3, line_len_64});

        for (int i = 1; i < 5; i++) {
            buffer[(line_len_64 + 2) * i - 2] = '\r';
            buffer[(line_len_64 + 2) * i - 1] = '\n';
        }

        if (!co_await file.write(buffer, sizeof(buffer)))
            co_return false;

        if (!co_await file.write(this->from.c_str(), this->from.size()))
            co_return false;

        if (!co_await file.write("\r\n", 2))
            co_return false;

        if (!co_await file.write(this->reverse_path.c_str(), this->reverse_path.size()))
            co_return false;

        if (!co_await file.write("\r\n", 2))
            co_return false;

        for (auto path : this->forward_paths) {
            if (!co_await file.write(path.c_str(), path.size()))
                co_return false;

            if (!co_await file.write("\r\n", 2))
                co_return false;
        }

        co_return true;
    }
}