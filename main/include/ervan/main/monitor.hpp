#pragma once

#include "ervan/coro.hpp"
#include "ervan/epoll.hpp"

namespace ervan::main {
    struct monitor_entry {
        std::function<coro<int>()> main;
        pid_t                      pid;
    };

    class monitor {
        public:
        monitor();

        void add(const char* name, std::function<coro<int>()> main);

        epoll_signal& get_epoll_handle() {
            return _epoll_signal;
        }

        private:
        epoll_signal               _epoll_signal;
        std::vector<monitor_entry> _entries;

        void  signal(signalfd_siginfo& info);
        pid_t enter(std::function<coro<int>()> func);
    };

}