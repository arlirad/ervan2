#include "eaio.hpp"
#include "eipc.hpp"
#include "ervan/config.hpp"
#include "ervan/ipc.hpp"
#include "ervan/log.hpp"
#include "ervan/main/monitor.hpp"
#include "ervan/smtp.hpp"
#include "ervan/string.hpp"

#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sstream>
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

    eaio::coro<void> handle_eipc(eipc::endpoint& ep, config_all& cfg) {
        for (;;) {
            auto request_try = co_await ep.recv_request();
            if (!request_try)
                break;

            auto request = request_try.value();

            if (request.function() == FUNC_READCONFIG) {
                auto key_list = request.get<config_key_list<>>();

                for (int i = 0; i < key_list.count; i++) {
                    config_update update;

                    update.key  = key_list.keys[i];
                    auto result = cfg.dump(key_list.keys[i], update.data, sizeof(update.data));

                    co_await ep.request_async(FUNC_WRITECONFIGKEY, update);
                }

                auto finalized = eipc::response();
                ep.respond(request, finalized);
            }
        }
    }

    eaio::coro<void> reload_config(eaio::dispatcher& d, config_all& cfg) {
        int fd = open(CONFIG_PATH, O_RDONLY);
        if (fd == -1) {
            log::out << log::format("Failed to open config ('{}'): {}.", CONFIG_PATH,
                                    strerror(errno));
            co_return;
        }

        auto    handle    = d.wrap<eaio::file>(fd);
        ssize_t file_size = handle.size();
        char*   buffer    = new char[file_size];
        size_t  offset    = 0;

        auto result = co_await handle.read(buffer, file_size);
        if (!result) {
            log::out << result.perror("Failed to read config.");
            co_return;
        }

        for (;;) {
            auto [key_start, key_end, value_start, value_end, new_offset, result] =
                get_config_pair(buffer, file_size, offset);
            if (result == PARSE_INVALID) {
                log::out << "Invalid config file.";
                break;
            }

            if (result == PARSE_END)
                break;

            offset = new_offset;

            std::string key(key_start, key_end);
            std::string value(value_start, value_end);

            if (!cfg.exists(key)) {
                log::out << log::format("Unknown config parameter '{}'.", key);
                continue;
            }

            if (!cfg.set(key, value)) {
                log::out << log::format("Invalid config parameter for '{}'.", key);
                break;
            }
        }

        delete buffer;
    }

    eaio::coro<int> main_async() {
        eipc::set_root("./eipc/");
        log::out.set_name("monitor");

        eaio::dispatcher dispatcher;
        eaio::signal     signal_reader;
        monitor          mon(dispatcher);
        eipc::endpoint   ep("monitor->smtp");
        config_all       main_config;
        sigset_t         mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigaddset(&mask, SIGPIPE);
        sigprocmask(SIG_BLOCK, &mask, nullptr);

        ep.init("smtp->monitor");

        auto ep_handle = wrap_endpoint(dispatcher, ep);

        co_await reload_config(dispatcher, main_config);

        dispatcher.spawn([&mon]() -> eaio::coro<void> { co_await mon.loop(); });
        dispatcher.spawn(handle_eipc, ep, main_config);

        if (main_config.ensure<config_debug>())
            mon.debug(main_config.get<config_debug>());

        mon.add("ervan.smtp", ervan::smtp::main_async);

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