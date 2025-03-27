#include "ervan/main/monitor.hpp"

#include "ervan/log.hpp"
#include <sys/prctl.h>
#include <sys/wait.h>

#include <format>
#include <functional>
#include <signal.h>
#include <unistd.h>


namespace ervan::main {
    monitor::monitor()
        : _epoll_signal(
              epoll_signal(SIGCHLD, [this](int, signalfd_siginfo& info) { this->signal(info); })) {}

    void monitor::add(const char* name, std::function<coro<int>()> main) {
        monitor_entry& entry = this->_entries.emplace_back(monitor_entry{
            .name = name,
            .main = main,
        });

        entry.pid = enter(entry.name, entry.main);
    }

    void monitor::signal(signalfd_siginfo& info) {
        for (monitor_entry& entry : this->_entries) {
            if (entry.pid != info.ssi_pid)
                continue;

            int exit_code;
            int pid = waitpid(info.ssi_pid, &exit_code, WNOHANG);

            log::out << std::format("Process '{}' ({}) terminated with code {}, signal '{}'.",
                                    entry.name, entry.pid, exit_code, strsignal(exit_code));

            entry.pid = enter(entry.name, entry.main);
        }
    }

    pid_t monitor::enter(const char* name, std::function<coro<int>()> main) {
        /*pid_t pid = fork();

        if (pid == 0) {
            prctl(PR_SET_NAME, name);
            main().handle.resume();
        }

        return pid;*/

        std::thread* debug_thread = new std::thread([main]() { main().handle.resume(); });
        return 0;
    }
}