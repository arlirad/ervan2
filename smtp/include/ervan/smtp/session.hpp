#pragma once

#include "ervan/message.hpp"
#include "ervan/smtp.hpp"
#include "ervan/smtp/types.hpp"
#include "ervan/string.hpp"

#include <algorithm>
#include <cstring>
#include <eaio.hpp>
#include <functional>
#include <optional>
#include <string>
#include <tuple>

namespace ervan::parse {
    struct esmtp_param_result;
}

namespace ervan::smtp {
    class session;

    using session_method = eaio::coro<void> (session::*)(span<char>);

    template <smtp_t_str N, session_method F>
    struct smtp_command {
        static constexpr smtp_t_str name                               = N;
        static constexpr eaio::coro<void> (session::*func)(span<char>) = F;
    };

    class session {
        public:
        session(eaio::socket& sock, eaio::dispatcher& d);

        eaio::coro<void> handle();

        private:
        enum state {
            STATE_CMD,
            STATE_DATA,
            STATE_CLOSED,
        };

        eaio::dispatcher& _d;
        eaio::socket&     _sock;
        state             _state;

        metadata _metadata;
        bool     _reverse_set;

        size_t      _max_data_size;
        size_t      _data_size;
        std::string _data_path;
        eaio::file  _data_file;
        size_t      _max_forward_paths;

        char       _cmd_buffer[512];
        int        _cmd_offset = 0;
        char       _line_buffer[1024];
        int        _line_offset = 0;
        span<char> _cmd_span    = span(_cmd_buffer, sizeof(_cmd_buffer));
        span<char> _line_span   = span(_line_buffer, sizeof(_line_buffer));

        bool _data_too_long;
        bool _data_error;
        bool _line_too_long;

        void reset();
        bool accept(span<char>& sp, const char* str);

        bool mail_param(parse::esmtp_param_result& param);
        bool rcpt_param(parse::esmtp_param_result& param);

        eaio::coro<void>             feed(span<const char> sp);
        eaio::coro<span<const char>> feed_cmd(span<const char> sp);
        eaio::coro<span<const char>> feed_data(span<const char> sp);

        eaio::coro<void> call_command(span<char> sp);
        eaio::coro<void> ehlo(span<char> sp);
        eaio::coro<void> helo(span<char> sp);
        eaio::coro<void> mail(span<char> sp);
        eaio::coro<void> rcpt(span<char> sp);
        eaio::coro<void> data(span<char> sp);
        eaio::coro<void> rset(span<char> sp);
        eaio::coro<void> vrfy(span<char> sp);
        eaio::coro<void> noop(span<char> sp);
        eaio::coro<void> quit(span<char> sp);

        eaio::coro<void> send_greeter();
        eaio::coro<void> reply(const char* str, size_t len);

        eaio::coro<bool> open_files();
        eaio::coro<bool> trace_data();
        eaio::coro<bool> write_header_space();
        eaio::coro<bool> write_data_tar_header(size_t length);
        eaio::coro<bool> write_metadata_tar_header(size_t length);
        eaio::coro<bool> pad_data();
        eaio::coro<void> write_data_to_disk();
        eaio::coro<void> finish_data();
        eaio::coro<void> abort_data();

        template <typename T>
        eaio::coro<void> reply(T m) {
            co_await this->reply(m.msg.str, sizeof(m.msg));
        }

        using cmds =
            std::tuple<smtp_command<"EHLO", &session::ehlo>, smtp_command<"HELO", &session::helo>,
                       smtp_command<"MAIL", &session::mail>, smtp_command<"RCPT", &session::rcpt>,
                       smtp_command<"DATA", &session::data>, smtp_command<"RSET", &session::rset>,
                       smtp_command<"VRFY", &session::vrfy>, smtp_command<"NOOP", &session::noop>,
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

    eaio::coro<void> handle(eaio::socket sock, eaio::dispatcher& d);
};