#include "ervan/main/monitor.hpp"

#include "ervan/log.hpp"
#include <sys/prctl.h>
#include <sys/wait.h>

#include <format>
#include <functional>
#include <signal.h>
#include <string.h>
#include <thread>
#include <unistd.h>

namespace ervan::main {
    monitor::monitor(eaio::dispatcher& d) : _d(d) {}

    void monitor::debug(bool v) {
        this->_debug = v;
    }

    void monitor::add(const char* name, std::function<eaio::coro<int>()> main) {
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

            log::out << log::format("Process '{}' ({}) terminated with code {}, signal '{}'.",
                                    entry.name, entry.pid, exit_code, strsignal(exit_code));

            entry.pid = enter(entry.name, entry.main);
        }
    }

    eaio::coro<void> monitor::loop() {
        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, nullptr);

        int  sfd    = signalfd(-1, &mask, SFD_CLOEXEC);
        auto signal = this->_d.wrap<eaio::signal>(sfd);

        for (;;) {
            auto result = co_await signal.get();
            if (!result) {
                log::out << "signalfd loop failed!";
                break;
            }

            this->signal(result.info);
        }
    }

    pid_t monitor::enter(const char* name, std::function<eaio::coro<int>()> main) {
        if (this->_debug) {
            std::thread* debug_thread = new std::thread([main]() { main().handle.resume(); });
            log::out << log::format("Spawned thread '{}'.", name);

            return 0;
        }

        pid_t pid = fork();

        if (pid == 0) {
            prctl(PR_SET_NAME, name);
            main().handle.resume();
        }

        log::out << log::format("Spawned process '{}' ({}).", name, pid);

        return pid;
    }
}