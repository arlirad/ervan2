#include "ervan/main/monitor.hpp"

#include <functional>
#include <signal.h>
#include <unistd.h>

namespace ervan::main {
    monitor::monitor()
        : _epoll_signal(
              epoll_signal(SIGCHLD, [this](int, signalfd_siginfo& info) { this->signal(info); })) {}

    void monitor::add(const char* name, std::function<coro<int>()> main) {
        monitor_entry& entry = this->_entries.emplace_back(monitor_entry{
            .main = main,
        });

        entry.pid = enter(entry.main);
    }

    void monitor::signal(signalfd_siginfo& info) {
        for (monitor_entry& entry : this->_entries) {
            if (entry.pid != info.ssi_pid)
                continue;

            entry.pid = enter(entry.main);
        }
    }

    pid_t monitor::enter(std::function<coro<int>()> main) {
        pid_t pid = fork();

        if (pid == 0)
            main().handle.resume();

        return pid;
    }
}