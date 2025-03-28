#pragma once

#include "eaio.hpp"

#include <functional>

namespace ervan::main {
    struct monitor_entry {
        const char*                      name;
        std::function<eaio::coro<int>()> main;
        pid_t                            pid;
    };

    class monitor {
        public:
        monitor(eaio::dispatcher& d);

        void             debug(bool v);
        void             add(const char* name, std::function<eaio::coro<int>()> main);
        eaio::coro<void> loop();

        private:
        eaio::dispatcher&          _d;
        std::vector<monitor_entry> _entries;
        bool                       _debug;

        void  signal(signalfd_siginfo& info);
        pid_t enter(const char* name, std::function<eaio::coro<int>()> func);
    };

}