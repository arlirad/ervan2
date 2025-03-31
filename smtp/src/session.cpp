#include "ervan/smtp/session.hpp"

#include "ervan/config.hpp"
#include "ervan/log.hpp"
#include "ervan/smtp.hpp"

#include <eaio.hpp>

namespace ervan::smtp {
    session::session(eaio::socket& sock) : _sock(sock) {}

    eaio::coro<void> session::read_command(span<const char> sp) {
        //
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

            this->feed(span(reinterpret_cast<const char*>(buffer), result.len));
        }
    }

    void session::feed(span<const char> sp) {
        if (_state == STATE_CMD)
            this->feed_cmd(sp);
        else if (_state == STATE_DATA)
            this->feed_data(sp);
    }

    void session::feed_cmd(span<const char> sp) {
        size_t len = std::min(sizeof(this->_cmd_buffer - this->_cmd_offset),
                              sp.len - sizeof(this->cmd_terminator));

        std::copy(sp.begin(), sp.begin() + len, this->_cmd_buffer);
        std::copy(sp.end() - sizeof(this->cmd_terminator), sp.end(),
                  this->_cmd_buffer + sizeof(this->_cmd_buffer) - sizeof(this->cmd_terminator));
    }

    void session::feed_data(span<const char> sp) {
        ;
    }

    eaio::coro<void> handle(eaio::socket sock) {
        session session(sock);
        co_await session.handle();
    }
}