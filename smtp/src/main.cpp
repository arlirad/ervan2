#include "eaio.hpp"
#include "eipc.hpp"
#include "ervan/log.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <format>
#include <unistd.h>

namespace ervan::smtp {
    eaio::dispatcher dispatcher;

    eaio::coro<void> handle(eaio::socket sock) {
        const char greeter[] = "220 www.example.com SMTP ready\r\n";

        co_await sock.send(greeter, sizeof(greeter));

        for (;;) {
            char buffer[32] = {};
            auto result     = co_await sock.recv(buffer, sizeof(buffer));

            if (!result) {
                log::out << result.perror("recv");
                break;
            }

            if (result.len == 0) {
                log::out << "done";
                break;
            }

            log::out << buffer;
        }
    }

    eaio::coro<void> listen(eaio::socket sock) {
        for (;;) {
            auto client_try = co_await sock.accept();
            if (!client_try) {
                log::out << client_try.perror("accept");
                co_return;
            }

            log::out << "New connection.";
            dispatcher.spawn(handle, client_try.returned_handle);
        }
    }

    eaio::coro<int> main_async() {
        mkdir("./ingoing/", 0700);
        mkdir("./outgoing/", 0700);

        log::out.set_name("smtp");

        sockaddr_in addr = {};

        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port        = htons(2525);

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

        for (;;)
            dispatcher.poll();

        co_return 0;
    }
}