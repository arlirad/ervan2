#include "ervan/epoll.hpp"
#include <sys/socket.h>

#include <fcntl.h>

namespace ervan {
    auto bsd_listen = listen;
    auto bsd_accept = accept;
    auto bsd_recv   = recv;

    epoll_socket::epoll_socket(int fd) {
        this->_fd = fd;
        fcntl(this->_fd, F_SETFL, fcntl(this->_fd, F_GETFL, 0) | O_NONBLOCK);
    }

    void epoll_socket::handle(uint32_t events) {
        if (events & EPOLLERR) {
            this->_recv_handle.notify(0);
            this->_send_handle.notify(0);

            return;
        }

        if (events & EPOLLIN)
            this->_recv_handle.notify(0);

        if (events & EPOLLOUT)
            this->_send_handle.notify(0);
    }

    int epoll_socket::listen(int backlog) {
        return bsd_listen(this->_fd, backlog);
    }

    coro<std::optional<epoll_socket>> epoll_socket::accept() {
        int new_fd;

        while ((new_fd = bsd_accept(this->_fd, nullptr, nullptr)) == -1) {
            if (errno == EWOULDBLOCK or errno == EAGAIN) {
                co_await this->_recv_handle;
                continue;
            }
            else {
                co_return {};
            }
        }

        fcntl(new_fd, F_SETFL, fcntl(new_fd, F_GETFL, 0) | O_NONBLOCK);
        co_return new_fd;
    }

    coro<ssize_t> epoll_socket::recv(void* buffer, size_t size) {
        ssize_t retval;

        while ((retval = bsd_recv(this->_fd, buffer, size, 0)) == -1) {
            if (errno == EWOULDBLOCK or errno == EAGAIN) {
                co_await this->_recv_handle;
                continue;
            }

            break;
        }

        co_return retval;
    }
}