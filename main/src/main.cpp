#include "eaio.hpp"
#include "eipc.hpp"
#include "ervan/log.hpp"
#include "ervan/main/monitor.hpp"
#include "ervan/smtp.hpp"

#include <signal.h>
#include <unistd.h>

namespace ervan::main {
    /*eaio::coro<void> interrupt_signal(eaio::dispatcher& d) {
        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigprocmask(SIG_BLOCK, &mask, nullptr);

        int  sfd    = signalfd(-1, &mask, SFD_CLOEXEC);
        auto signal = d.wrap<eaio::signal>(sfd);

        co_await signal.get();
        log::out << "Got sigint.";
    }*/

    eaio::coro<int> main_async() {
        eaio::dispatcher dispatcher;
        eaio::signal     signal_reader;
        monitor          mon(dispatcher);
        sigset_t         mask;

        eipc::set_root("./eipc/");
        log::out.set_name("monitor");

        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, nullptr);

        mon.debug(true);
        mon.add("ervan.smtp", ervan::smtp::main_async);

        dispatcher.spawn([&mon]() -> eaio::coro<void> { co_await mon.loop(); });
        // dispatcher.spawn(interrupt_signal, dispatcher);

        for (;;)
            dispatcher.poll();

        co_return 0;
    }

    extern "C" int main() {
        auto a = main_async();
        a.handle.resume();
        return a.get_value();
    }
}