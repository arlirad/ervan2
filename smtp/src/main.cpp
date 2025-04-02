#include "eaio.hpp"
#include "eipc.hpp"
#include "ervan/config.hpp"
#include "ervan/io.hpp"
#include "ervan/ipc.hpp"
#include "ervan/log.hpp"
#include "ervan/smtp.hpp"
#include "ervan/smtp/session.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <cstdlib>
#include <format>
#include <unistd.h>

namespace ervan::smtp {
    eaio::dispatcher       dispatcher;
    constinit local_config cfg;

    eaio::coro<void> handle_eipc(eipc::endpoint& ep) {
        for (;;) {
            auto request_try = co_await ep.recv_request();
            if (!request_try)
                break;

            auto request = request_try.value();

            if (request.function() == FUNC_WRITECONFIGKEY) {
                auto update = request.get<config_update>();

                cfg.set(update.key, update.data, sizeof(update.data));

                eipc::response ack;
                ep.respond(request, ack);
            }
        }
    }

    eaio::coro<void> listen(eaio::socket sock) {
        if (!cfg.ensure<config_hostname>()) {
            log::out << "Hostname is not set!";
            co_return;
        }

        log::out << log::format("Listening as '{}' on port {} and {}.", cfg.get<config_hostname>(),
                                cfg.get<config_port>(), cfg.get<config_submissionport>());
        log::out << log::format("Max message size: {} KiB.",
                                cfg.get<config_maxmessagesize>() / 1024);
        log::out << log::format("Max recipients per message: {}.", cfg.get<config_maxrcpt>());

        for (;;) {
            auto client_try = co_await sock.accept();
            if (!client_try) {
                log::out << client_try.perror("accept");
                co_return;
            }

            log::out << "New connection.";
            dispatcher.spawn(handle, client_try.returned_handle, dispatcher);
        }
    }

    eaio::coro<void> start(eipc::endpoint& ep) {
        dispatcher.spawn(handle_eipc, ep);

        co_await ep.request_async(FUNC_READCONFIG, cfg.key_list);

        sockaddr_in addr = {};

        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port        = htons(cfg.get<config_port>());

        int  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        auto sock    = dispatcher.wrap<eaio::socket>(sock_fd);

        auto setsockopt_result = sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, true);
        if (!setsockopt_result)
            log::out << setsockopt_result.perror("setsockopt");

        auto bind_result = sock.bind(addr);
        if (!bind_result)
            log::out << bind_result.perror("bind");

        auto listen_result = sock.listen(32);
        if (!listen_result)
            log::out << listen_result.perror("listen");

        dispatcher.spawn(listen, sock);
    }

    eaio::coro<int> main_async() {
        srand(time(nullptr));
        io::install_io_signal();

        eipc::endpoint ep("smtp->monitor");

        mkdir("./ingoing/", 0700);
        mkdir("./outgoing/", 0700);

        log::out.set_name("smtp");

        ep.init("monitor->smtp");

        dispatcher.spawn(start, ep);

        auto ep_wrapped = wrap_endpoint(dispatcher, ep);

        for (;;)
            dispatcher.poll();

        co_return 0;
    }
}