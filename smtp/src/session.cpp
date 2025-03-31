#include "ervan/smtp/session.hpp"

#include "ervan/config.hpp"
#include "ervan/log.hpp"
#include "ervan/smtp.hpp"

#include <eaio.hpp>

namespace ervan::smtp {
    session::session(eaio::socket& sock) : _sock(sock) {}

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

        co_await (this->*result.value())();
    }

    eaio::coro<void> send_greeter(eaio::socket& sock) {
        auto hostname = cfg.get<config_hostname>();
        auto a        = sock.send("220 ", 4);
        auto b        = sock.send(hostname.c_str(), hostname.size());
        auto c        = sock.send(" SMTP Ervan 2 ready\r\n", 22);

        co_await a;
        co_await b;
        co_await c;
    }

    eaio::coro<void> session::handle() {
        co_await send_greeter(this->_sock);

        for (;;) {
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
    }

    eaio::coro<void> session::feed(span<const char> sp) {
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
        ;
    }

    eaio::coro<void> session::ehlo() {
        log::out << "ehlo";
        co_return;
    }

    eaio::coro<void> session::mail() {
        log::out << "mail";
        co_return;
    }

    eaio::coro<void> session::reply(const char* str, size_t len) {
        co_await this->_sock.send(str, len);
    }

    eaio::coro<void> handle(eaio::socket sock) {
        session session(sock);
        co_await session.handle();
    }
}