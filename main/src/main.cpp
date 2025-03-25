#include "ervan/coro.hpp"
#include "ervan/epoll.hpp"
#include "ervan/log.hpp"
#include "ervan/main/monitor.hpp"
#include "ervan/smtp.hpp"

#include <signal.h>
#include <unistd.h>

namespace ervan::main {
    coro<int> main_async() {
        monitor       mon;
        epoll_handler handler;
        epoll_signal& signal_handle = mon.get_epoll_handle();
        sigset_t      mask;

        eipc::set_root("./eipc/");
        log::out.set_name("monitor");

        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, nullptr);

        handler.add(&signal_handle);
        mon.add("ervan.smtp", smtp::main_async);

        for (;;)
            handler.poll();

        co_return 0;
    }

    extern "C" int main() {
        auto main_coro = main_async();

        main_coro.handle.resume();

        return main_coro.get_value();
    }
}