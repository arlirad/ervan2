#include "ervan/io.hpp"
#include "ervan/smtp/session.hpp"
#include <sys/stat.h>

#include <fcntl.h>

namespace ervan::smtp {
    bool session::open_files() {
        std::string path = "./ingoing/" + get_unique_filename("msg");

        this->_data_path     = (path + ".mbox").c_str();
        this->_metadata_path = (path + ".ervan").c_str();

        int fd_data = open(this->_data_path.c_str(), O_CREAT | O_WRONLY);
        if (fd_data == -1)
            return false;

        fchmod(fd_data, S_IRUSR | S_IWUSR | S_IRGRP);

        int fd_metadata = open(this->_metadata_path.c_str(), O_CREAT | O_WRONLY);
        if (fd_metadata == -1) {
            if (fd_data != -1) {
                unlink(this->_data_path.c_str());
                close(fd_data);
            }

            return false;
        }

        fchmod(fd_metadata, S_IRUSR | S_IWUSR | S_IRGRP);

        this->_data_file     = this->_d.wrap<eaio::file>(fd_data);
        this->_metadata_file = this->_d.wrap<eaio::file>(fd_metadata);

        return true;
    }

    eaio::coro<void> session::finish_data() {
        auto write_metadata = this->_metadata.write_to(this->_metadata_file);
        auto data_sync      = io::sync_file(this->_data_file);

        co_await data_sync;
        co_await write_metadata;

        this->_data_file.close();
        this->_data_file = {};
        this->_data_path = {};
        this->_metadata_file.close();
        this->_metadata_file = {};
        this->_metadata_path = {};

        this->_state = STATE_CMD;
        co_await this->reply(ok);
    }

    eaio::coro<void> session::abort_data() {
        ::unlink(this->_data_path.c_str());
        ::unlink(this->_metadata_path.c_str());

        this->_data_file.truncate(0);
        this->_data_file.close();
        this->_data_file = {};
        this->_data_path = {};
        this->_metadata_file.truncate(0);
        this->_metadata_file.close();
        this->_metadata_file = {};
        this->_metadata_path = {};

        this->_state = STATE_CMD;

        if (this->_data_too_long)
            co_await this->reply(exceeded_storage);
        else if (this->_line_too_long)
            co_await this->reply(exceeded_line);
    }
}