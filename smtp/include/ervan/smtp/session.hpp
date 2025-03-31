#pragma once

#include "ervan/string.hpp"

#include <algorithm>
#include <cstring>
#include <eaio.hpp>
#include <functional>
#include <optional>
#include <string>
#include <tuple>

namespace ervan::smtp {
    class session;

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

    using session_method = eaio::coro<void> (session::*)(span<char>);

    template <smtp_t_str N, session_method F>
    struct smtp_command {
        static constexpr smtp_t_str name                               = N;
        static constexpr eaio::coro<void> (session::*func)(span<char>) = F;
    };

    template <int code, smtp_t_str M>
    struct smtp_reply {
        static constexpr smtp_msg_str msg = smtp_msg_str(code, M.str);
    };

    const smtp_reply<250, "OK">                                       ok;
    const smtp_reply<354, "Start mail input; end with <CRLF>.<CRLF>"> start_data;
    const smtp_reply<500, "Syntax error, command unrecognized">       invalid_command;
    const smtp_reply<501, "Syntax error in parameters or arguments">  invalid_parameters;

    class session {
        public:
        session(eaio::socket& sock);

        eaio::coro<void> handle();

        private:
        enum state {
            STATE_CMD,
            STATE_DATA,
            STATE_CLOSED,
        };

        span<const char> cmd_terminator  = span("\r\n", 2);
        span<const char> data_terminator = span("\r\n.\r\n", 5);
        eaio::socket&    _sock;
        state            _state;
        char             _cmd_buffer[512];
        int              _cmd_offset = 0;
        char             _data_term_buffer[5];
        int              _data_length = 0;
        span<char> _cmd_span = span(_cmd_buffer, sizeof(_cmd_buffer) - sizeof(cmd_terminator));
        span<char> _data_term_span = span(_data_term_buffer, sizeof(_data_term_buffer));

        void reset();
        bool accept(span<char>& sp, const char* str);

        eaio::coro<void> feed(span<const char> sp);
        eaio::coro<void> feed_cmd(span<const char> sp);
        eaio::coro<void> feed_data(span<const char> sp);

        eaio::coro<void> call_command(span<char> sp);
        eaio::coro<void> ehlo(span<char> sp);
        eaio::coro<void> mail(span<char> sp);
        eaio::coro<void> rcpt(span<char> sp);
        eaio::coro<void> data(span<char> sp);
        eaio::coro<void> quit(span<char> sp);

        eaio::coro<void> reply(const char* str, size_t len);

        template <typename T>
        eaio::coro<void> reply(T m) {
            co_await this->reply(m.msg.str, sizeof(m.msg));
        }

        using cmds =
            std::tuple<smtp_command<"EHLO", &session::ehlo>, smtp_command<"MAIL", &session::mail>,
                       smtp_command<"RCPT", &session::rcpt>, smtp_command<"DATA", &session::data>,
                       smtp_command<"QUIT", &session::quit>>;
        cmds _commands;

        template <typename C>
        constexpr bool compare(const char* compared_to) {
            return std::strcmp(C::name.str, compared_to) == 0;
        }

        template <int I = 0>
        std::optional<session_method> try_get_command(const char* str) {
            if constexpr (I < std::tuple_size_v<cmds>)
                return this->compare<typename std::tuple_element<I, cmds>::type>(str)
                           ? std::get<I>(this->_commands).func
                           : try_get_command<I + 1>(str);

            return {};
        }
    };

    eaio::coro<void> handle(eaio::socket sock);
};