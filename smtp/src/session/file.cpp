#include "ervan/io.hpp"
#include "ervan/smtp/session.hpp"
#include <sys/stat.h>

#include <fcntl.h>

namespace ervan::smtp {
    eaio::coro<bool> session::open_files() {
        std::string path = "./ingoing/" + get_unique_filename("msg");

        this->_data_path = (path + ".tar").c_str();

        int fd_data = open(this->_data_path.c_str(), O_CREAT | O_WRONLY);
        if (fd_data == -1)
            co_return false;

        fchmod(fd_data, S_IRUSR | S_IWUSR | S_IRGRP);

        this->_data_file = this->_d.wrap<eaio::file>(fd_data);

        co_await this->write_header_space();
        co_return true;
    }

    eaio::coro<void> session::write_header_space() {
        char header[512] = {};
        co_await this->_data_file.write(header, sizeof(header));
    }

    eaio::coro<void> session::write_data_tar_header(size_t length) {
        io::tar_header hdr = io::tar_header::filled("message.mbox", length);

        co_await this->_data_file.write(&hdr, sizeof(hdr));
        co_return;
    }

    eaio::coro<void> session::write_metadata_tar_header(size_t length) {
        io::tar_header hdr = io::tar_header::filled("metadata.ervan", length);

        co_await this->_data_file.write(&hdr, sizeof(hdr));
        co_return;
    }

    eaio::coro<void> session::pad_data() {
        ssize_t file_size   = this->_data_file.tellg();
        char    buffer[512] = {};

        co_await this->_data_file.write(buffer, 512 - file_size % sizeof(buffer));
        co_return;
    }

    eaio::coro<void> session::finish_data() {
        ssize_t file_size = this->_data_file.tellg();
        if (file_size < 0) {
            this->_state = STATE_CMD;
            co_await this->reply(local_error);
            co_return;
        }

        this->_data_file.seekg(0, SEEK_SET);
        co_await this->write_data_tar_header(file_size - 512);
        this->_data_file.seekg(0, SEEK_END);
        co_await this->pad_data();

        size_t data_size = this->_data_file.tellg();
        co_await this->write_header_space();

        size_t pre_metadata_size = this->_data_file.tellg();
        co_await this->_metadata.write_to(this->_data_file);

        size_t total_size = this->_data_file.tellg();

        this->_data_file.seekg(data_size, SEEK_SET);
        co_await this->write_metadata_tar_header(total_size - pre_metadata_size);
        this->_data_file.seekg(0, SEEK_END);
        co_await this->pad_data();

        auto data_sync = io::sync_file(this->_data_file);

        co_await data_sync;

        this->_data_file.close();
        this->_data_file = {};
        this->_data_path = {};

        this->_state = STATE_CMD;
        co_await this->reply(ok);
    }

    eaio::coro<void> session::abort_data() {
        ::unlink(this->_data_path.c_str());

        this->_data_file.truncate(0);
        this->_data_file.close();
        this->_data_file = {};
        this->_data_path = {};

        this->_state = STATE_CMD;

        if (this->_data_too_long)
            co_await this->reply(exceeded_storage);
        else if (this->_line_too_long)
            co_await this->reply(exceeded_line);
    }
}