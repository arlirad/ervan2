#include "eipc.hpp"
#include "ervan/coro.hpp"
#include "ervan/epoll.hpp"
#include "ervan/log.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <format>
#include <unistd.h>

namespace ervan::smtp {
    epoll_handler handler;

    coro<int> handle(epoll_socket sock) {
        log::out << "handling";
        handler.add(&sock);

        for (;;) {
            char    buffer[32] = {};
            ssize_t len        = co_await sock.recv(buffer, sizeof(buffer));

            if (len == 0)
                break;

            log::out << buffer;
        }

        handler.remove(&sock);

        co_return 0;
    }

    coro<int> listen(epoll_socket& sock) {
        handler.add(&sock);

        for (;;) {
            auto client_try = co_await sock.accept();
            if (!client_try.has_value()) {
                log::out << std::format("accept: {}", strerror(errno));
                co_return 0;
            }

            handler.spawn(handle, client_try.value());
        }

        handler.remove(&sock);
    }

    coro<int> main_async() {
        mkdir("./ingoing/", 0700);
        mkdir("./outgoing/", 0700);

        log::out.set_name("smtp");

        sockaddr_in addr = {};

        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port        = htons(2525);

        int sock   = socket(AF_INET, SOCK_STREAM, 0);
        int enable = 1;

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

        int bind_result = bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        log::out << std::format("bind: {} ({})", bind_result, strerror(errno));

        epoll_socket listen_sock(sock);
        int          listen_result = listen_sock.listen(10);

        log::out << std::format("listen: {} ({})", bind_result, strerror(errno));

        handler.spawn(listen, listen_sock);

        for (;;)
            handler.poll();

        co_return 0;
    }
}