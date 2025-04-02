#include "ervan/smtp/session.hpp"

#include "ervan/config.hpp"
#include "ervan/log.hpp"
#include "ervan/parse.hpp"
#include "ervan/smtp.hpp"

#include <eaio.hpp>

namespace ervan::smtp {
    session::session(eaio::socket& sock, eaio::dispatcher& d) : _sock(sock), _d(d) {
        this->reset();
    }

    eaio::coro<void> session::handle() {
        co_await this->send_greeter();

        while (this->_state != STATE_CLOSED) {
            char buffer[8192];
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

    void session::reset() {
        this->_state = STATE_CMD;

        this->_max_data_size     = cfg.get<config_maxmessagesize>();
        this->_max_forward_paths = cfg.get<config_maxrcpt>();
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

    eaio::coro<void> session::send_greeter() {
        auto hostname = cfg.get<config_hostname>();
        co_await this->_sock.send("220 ", 4);
        co_await this->_sock.send(hostname.c_str(), hostname.size());
        co_await this->_sock.send(" SMTP Ervan 2 ready\r\n", 21);
    }

    eaio::coro<void> session::reply(const char* str, size_t len) {
        co_await this->_sock.send(str, len);
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

        if (result.ec == std::errc::result_out_of_range)
            co_await this->reply(exceeded_line);

        if (result.rest.size() > 0)
            co_await this->feed(result.rest);
    }

    eaio::coro<void> session::feed_data(span<const char> sp) {
        join_result result;

        while (true) {
            result = join(this->_line_span, sp, this->_line_offset, cmd_terminator);
            sp     = result.rest;

            if (!result.sp.begin()) {
                if (result.ec == std::errc::result_out_of_range)
                    this->_line_too_long = true;

                break;
            }

            span line = result.sp;

            if (std::strcmp(result.sp.begin(), ".") == 0) {
                if (!this->_line_too_long && !this->_data_too_long)
                    co_await this->finish_data();
                else
                    co_await this->abort_data();

                break;
            }

            if (line[0] == '.' && line[1] == '.')
                line = span(line.begin() + 1, line.end());

            this->_data_size += line.size() + 2;

            if (line.size() > max_line_length - 2)
                this->_line_too_long = true;

            if (this->_data_size > this->_max_data_size)
                this->_data_too_long = true;

            if (this->_line_too_long || this->_data_too_long)
                continue;

            *(line.end() + 0) = '\r';
            *(line.end() + 1) = '\n';
            *(line.end() + 2) = '\0';

            line = span(line.begin(), line.end() + 2);

            co_await this->_data_file.write(line.begin(), line.size());
        }

        if (result.rest.size() > 0)
            co_await this->feed(result.rest);
    }

    eaio::coro<void> handle(eaio::socket sock, eaio::dispatcher& d) {
        session session(sock, d);
        co_await session.handle();
    }
}