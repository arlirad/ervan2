#include "ervan/smtp/session.hpp"

#include "ervan/config.hpp"
#include "ervan/log.hpp"
#include "ervan/smtp.hpp"

#include <eaio.hpp>

namespace ervan::smtp {
    session::session(eaio::socket& sock) : _sock(sock) {
        this->reset();
    }

    void session::reset() {
        this->_state = STATE_CMD;
    }

    eaio::coro<void> session::call_command(span<char> sp) {
        if (sp.size() < 4) {
            co_await this->reply(invalid_command);
            co_return;
        }

        sp[0] = std::toupper(sp[0]);
        sp[1] = std::toupper(sp[1]);
        sp[2] = std::toupper(sp[2]);
        sp[3] = std::toupper(sp[3]);
        sp[4] = '\0';

        auto result = this->try_get_command(sp.begin());
        if (!result) {
            co_await this->reply(invalid_command);
            co_return;
        }

        sp = span(sp.begin() + 5, sp.end());

        co_await (this->*result.value())(sp);
    }

    eaio::coro<void> send_greeter(eaio::socket& sock) {
        auto hostname = cfg.get<config_hostname>();
        co_await sock.send("220 ", 4);
        co_await sock.send(hostname.c_str(), hostname.size());
        co_await sock.send(" SMTP Ervan 2 ready\r\n", 21);
    }

    eaio::coro<void> session::handle() {
        co_await send_greeter(this->_sock);

        while (this->_state != STATE_CLOSED) {
            char buffer[4096];
            auto result = co_await this->_sock.recv(buffer, sizeof(buffer));

            if (!result) {
                log::out << result.perror("recv");
                break;
            }

            if (result.len == 0)
                break;

            co_await this->feed(span(reinterpret_cast<const char*>(buffer), result.len));
        }

        this->_sock.shutdown(SHUT_RDWR);
        log::out << "Done.";
    }

    bool session::accept(span<char>& sp, const char* str) {
        size_t len = std::strlen(str);

        if (sp.size() < len)
            return false;

        for (int i = 0; i < len; i++)
            sp[i] = toupper(str[i]);

        if (std::memcmp(sp.begin(), str, len) != 0)
            return false;

        sp = span(sp.begin() + len, sp.end());

        return true;
    }

    eaio::coro<void> session::feed(span<const char> sp) {
        if (this->_state == STATE_CLOSED)
            co_return;

        if (_state == STATE_CMD)
            co_await this->feed_cmd(sp);
        else if (_state == STATE_DATA)
            co_await this->feed_data(sp);
    }

    eaio::coro<void> session::feed_cmd(span<const char> sp) {
        auto result = join(this->_cmd_span, sp, _cmd_offset, cmd_terminator);

        if (result.sp.begin()) {
            log::out << result.sp.begin();
            co_await this->call_command(result.sp);
        }

        if (result.rest.size() > 0)
            co_await this->feed(result.rest);
    }

    eaio::coro<void> session::feed_data(span<const char> sp) {
        auto result =
            look(this->_data_term_span, sp, 34524, this->_data_length, this->data_terminator);

        if (result.sp.begin()) {
            this->_state = STATE_CMD;
            co_await this->reply(ok);
        }

        if (result.rest.size() > 0)
            co_await this->feed(result.rest);
    }

    eaio::coro<void> session::ehlo(span<char> sp) {
        auto hostname = cfg.get<config_hostname>();

        co_await this->_sock.send("250 ", 4);
        co_await this->_sock.send(hostname.c_str(), hostname.size());
        co_await this->_sock.send(" at your service\r\n", 18);
    }

    eaio::coro<void> session::mail(span<char> sp) {
        if (!accept(sp, "FROM:")) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        log::out << sp.begin();
        co_await this->reply(ok);
    }

    eaio::coro<void> session::rcpt(span<char> sp) {
        if (!accept(sp, "TO:")) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        log::out << sp.begin();
        co_await this->reply(ok);
    }

    eaio::coro<void> session::data(span<char> sp) {
        this->_state = STATE_DATA;
        co_await this->reply(start_data);
    }

    eaio::coro<void> session::quit(span<char> sp) {
        auto hostname = cfg.get<config_hostname>();

        co_await this->_sock.send("221 ", 4);
        co_await this->_sock.send(hostname.c_str(), hostname.size());
        co_await this->_sock.send(" do widzenia\r\n", 14);

        this->_state = STATE_CLOSED;
    }

    eaio::coro<void> session::reply(const char* str, size_t len) {
        co_await this->_sock.send(str, len);
    }

    eaio::coro<void> handle(eaio::socket sock) {
        session session(sock);
        co_await session.handle();
    }
}