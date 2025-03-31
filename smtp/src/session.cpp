#include "ervan/smtp/session.hpp"

#include "ervan/config.hpp"
#include "ervan/log.hpp"
#include "ervan/smtp.hpp"
#include <sys/stat.h>

#include <eaio.hpp>
#include <fcntl.h>

namespace ervan::smtp {
    session::session(eaio::socket& sock, eaio::dispatcher& d) : _sock(sock), _d(d) {
        this->reset();
    }

    void session::reset() {
        this->_state           = STATE_CMD;
        this->_max_data_length = cfg.get<config_maxmessagesize>();
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

    bool session::open_files() {
        std::string data_path     = (this->_message_path + ".mbox").c_str();
        std::string metadata_path = (this->_message_path + ".ervan").c_str();

        int fd_data = open(data_path.c_str(), O_CREAT | O_WRONLY);
        if (fd_data == -1)
            return false;

        fchmod(fd_data, S_IRUSR | S_IWUSR | S_IRGRP);

        int fd_metadata = open(metadata_path.c_str(), O_CREAT | O_WRONLY);
        if (fd_metadata == -1) {
            if (fd_data != -1) {
                unlink(metadata_path.c_str());
                close(fd_data);
            }

            return false;
        }

        fchmod(fd_metadata, S_IRUSR | S_IWUSR | S_IRGRP);

        this->_message_file  = this->_d.wrap<eaio::file>(fd_data);
        this->_metadata_file = this->_d.wrap<eaio::file>(fd_metadata);

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
        size_t remaining_space = get_remaining_msg_space();
        auto   result = look(this->_data_term_span, sp, this->_max_data_length, this->_data_length,
                             data_terminator, terminator_escape);

        if (result.ec == std::errc::result_out_of_range)
            co_await this->abort_data();

        if (result.sp.begin() && !result.escape) {
            span remaining = span<const char>(sp.begin(), result.rest.begin());

            if (remaining_space + 5 < remaining.size()) {
                co_await this->abort_data();
            }
            else {
                co_await this->_message_file.write(remaining.begin(), remaining.size());
                co_await this->finish_data();
            }
        }
        else if (result.escape && remaining_space > 0) {
            co_await this->_message_file.write(result.sp.begin(),
                                               std::min(result.sp.size(), remaining_space));
        }
        else if (remaining_space > 0) {
            co_await this->_message_file.write(sp.begin(), std::min(sp.size(), remaining_space));
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

        co_await this->reply(ok);
    }

    eaio::coro<void> session::rcpt(span<char> sp) {
        if (!accept(sp, "TO:")) {
            co_await this->reply(invalid_parameters);
            co_return;
        }

        co_await this->reply(ok);
    }

    eaio::coro<void> session::data(span<char> sp) {
        this->_message_path = "./ingoing/" + get_unique_filename("msg");

        if (!this->open_files()) {
            co_await this->reply(local_error);
            co_return;
        }

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

    eaio::coro<void> session::finish_data() {
        this->_message_file.truncate(this->_message_file.size() - 5);
        this->_message_file.close();
        this->_message_file = {};
        this->_message_path = {};

        this->_state = STATE_CMD;
        co_await this->reply(ok);
    }

    eaio::coro<void> session::abort_data() {
        ::unlink(this->_message_path.c_str());

        this->_message_file.truncate(0);
        this->_message_file.close();
        this->_message_file = {};
        this->_message_path = {};

        this->_state = STATE_CMD;
        co_await this->reply(exceeded_storage);
    }

    size_t session::get_remaining_msg_space() {
        if (this->_data_length > this->_max_data_length)
            return 0;

        return this->_max_data_length - this->_data_length;
    }

    eaio::coro<void> handle(eaio::socket sock, eaio::dispatcher& d) {
        session session(sock, d);
        co_await session.handle();
    }
}