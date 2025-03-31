#pragma once

#include "ervan/string.hpp"

#include <eaio.hpp>

namespace ervan::smtp {
    class session {
        public:
        session(eaio::socket& sock);

        eaio::coro<void> handle();

        private:
        enum state {
            STATE_CMD,
            STATE_DATA,
        };

        const char cmd_terminator[2]  = {'\r', '\n'};
        const char data_terminator[5] = {
            '\r', '\n', '.', '\r', '\n',
        };

        eaio::socket& _sock;
        state         _state;
        char          _cmd_buffer[510 + sizeof(cmd_terminator)];
        int           _cmd_offset = 0;

        void reset();

        void feed(span<const char> sp);
        void feed_cmd(span<const char> sp);
        void feed_data(span<const char> sp);

        eaio::coro<void> read_command(span<const char> previous);
    };

    eaio::coro<void> handle(eaio::socket sock);
};