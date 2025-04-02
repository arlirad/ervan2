#include "ervan/parse.hpp"
#include "ervan/smtp/session.hpp"

namespace ervan::smtp {
    eaio::coro<void> session::ehlo(span<char> sp) {
        auto hostname = cfg.get<config_hostname>();
        // auto size     = cfg.get<config_maxmessagesize>();
        // auto size_str = std::format("{}", size);

        co_await this->_sock.send("250-", 4);
        co_await this->_sock.send(hostname.c_str(), hostname.size());
        co_await this->_sock.send(" at your service\r\n", 18);
        co_await this->_sock.send("250 8BITMIME\r\n", 14);
        /*co_await this->_sock.send("250 SIZE ", 9);
        co_await this->_sock.send(size_str.c_str(), size_str.size());
        co_await this->_sock.send("\r\n", 2);*/
    }

    eaio::coro<void> session::mail(span<char> sp) {
        if (!accept(sp, "FROM:")) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        auto cmd     = span<const char>(sp.begin(), sp.end());
        auto reverse = parse::reverse_path(cmd);
        if (!reverse) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        cmd = reverse.rest;

        while (cmd.size() > 0) {
            if (cmd[0] != ' ') {
                co_await this->reply(invalid_parameters);
                co_return;
            }

            cmd = cmd.advance(1);

            auto parameter = parse::esmtp_param(cmd);
            if (!parameter) {
                co_await this->reply(invalid_parameters);
                co_return;
            }

            if (!this->mail_param(parameter)) {
                co_await this->reply(invalid_parameters);
                co_return;
            }

            cmd = parameter.rest;
        }

        this->_metadata = {};

        this->_metadata.reverse_path =
            std::string(reverse.mailbox.local_part.body.begin(), reverse.mailbox.domain.body.end());

        co_await this->reply(ok);
    }

    bool session::mail_param(parse::esmtp_param_result& param) {
        if (param.key.body.compare_insensitive(span("BODY", 4))) {
            if (param.value.body == span("7BIT", 4))
                return true;
            else if (param.value.body == span("8BITMIME", 8))
                return true;
            else
                return false;
        }

        return false;
    }

    eaio::coro<void> session::rcpt(span<char> sp) {
        if (!accept(sp, "TO:")) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        if (this->_metadata.forward_paths.size() >= this->_max_forward_paths) {
            co_await this->reply(too_many_recipients);
            co_return;
        }

        auto cmd  = span<const char>(sp.begin(), sp.end());
        auto path = parse::path(cmd);
        if (!path) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        this->_metadata.forward_paths.push_back(
            {path.mailbox.local_part.body.begin(), path.mailbox.domain.body.end()});

        co_await this->reply(ok);
    }

    bool session::rcpt_param(parse::esmtp_param_result& param) {
        return false;
    }

    eaio::coro<void> session::data(span<char> sp) {
        if (!co_await this->open_files()) {
            co_await this->reply(local_error);
            co_return;
        }

        this->_state         = STATE_DATA;
        this->_data_size     = 0;
        this->_data_too_long = false;
        this->_line_too_long = false;

        co_await this->reply(start_data);
    }

    eaio::coro<void> session::quit(span<char> sp) {
        auto hostname = cfg.get<config_hostname>();

        co_await this->_sock.send("221 ", 4);
        co_await this->_sock.send(hostname.c_str(), hostname.size());
        co_await this->_sock.send(" do widzenia\r\n", 14);

        this->_state = STATE_CLOSED;
    }
}