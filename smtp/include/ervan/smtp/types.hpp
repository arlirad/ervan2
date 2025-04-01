#pragma once

#include "ervan/string.hpp"

#include <unistd.h>

namespace ervan::smtp {
    template <size_t N>
    struct smtp_t_str {
        char str[N]{};

        constexpr smtp_t_str(const char (&str)[N]) {
            std::copy_n(str, N, this->str);
        }
    };

    template <size_t N>
    struct smtp_msg_str {
        char str[N + 6]{};

        constexpr smtp_msg_str(int code, const char (&str)[N]) {
            this->str[0] = '0' + ((code / 100) % 10);
            this->str[1] = '0' + ((code / 10) % 10);
            this->str[2] = '0' + ((code / 1) % 10);
            this->str[3] = ' ';

            std::copy_n(str, N, this->str + 4);

            this->str[N + 3] = '\r';
            this->str[N + 4] = '\n';
            this->str[N + 5] = '\0';
        }
    };

    template <int code, smtp_t_str M>
    struct smtp_reply {
        static constexpr smtp_msg_str msg = smtp_msg_str(code, M.str);
    };

    const smtp_reply<250, "OK">                                                  ok;
    const smtp_reply<354, "Start mail input; end with <CRLF>.<CRLF>">            start_data;
    const smtp_reply<451, "Requested action aborted: local error in processing"> local_error;
    const smtp_reply<500, "Syntax error, command unrecognized">                  invalid_command;
    const smtp_reply<500, "Line too long">                                       exceeded_line;
    const smtp_reply<501, "Syntax error in parameters or arguments">             invalid_parameters;
    const smtp_reply<552, "Requested mail action aborted: exceeded storage allocation">
        exceeded_storage;

    const span<const char> cmd_terminator    = span("\r\n", 2);
    const span<const char> data_terminator   = span("\r\n.\r\n", 5);
    const span<const char> terminator_escape = span("\r\n..", 4);

    const size_t max_line_length = 1000;
}