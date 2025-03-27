#pragma once

#include "eipc.hpp"
#include "ervan/coro.hpp"
#include <sys/epoll.h>
#include <sys/signalfd.h>

#include <functional>
#include <optional>

namespace ervan {
    class epoll_handle;

    class epoll_handler {
        public:
        static const int MAX_EVENTS = 32;

        epoll_handler();

        template <typename T, typename... U>
        void spawn(T&& f, U&&... args) {
            call_def(f, args...);
        }

        void add(epoll_handle* handle);
        void remove(epoll_handle* handle);
        void poll();

        private:
        int                                  _fd;
        epoll_event                          _events[MAX_EVENTS];
        std::vector<epoll_handle*>           _handles;
        std::vector<std::coroutine_handle<>> to_cleanup;

        template <typename T, typename... U>
        coro<deferred> call_def(T&& f, U&&... args) {
            auto c = f(args...);
            co_await c;

            auto handle = co_await coro_get_handle();
            to_cleanup.push_back(handle);
        }
    };

    class epoll_handle {
        public:
        virtual int  get_fd()                = 0;
        virtual void handle(uint32_t events) = 0;
    };

    class epoll_endpoint : public epoll_handle {
        public:
        epoll_endpoint(eipc::endpoint& ep) : _ep(ep) {}

        constexpr int get_fd() {
            return this->_ep.native_handle;
        }

        void handle(uint32_t events) {
            if (!(events & EPOLLIN))
                return;

            while (this->_ep.try_receive())
                ;
        }

        private:
        eipc::endpoint& _ep;
    };

    class epoll_signal : public epoll_handle {
        public:
        epoll_signal(int sig_id, std::function<void(int, signalfd_siginfo&)> cb);

        constexpr int get_fd() {
            return _fd;
        }

        void handle(uint32_t events);

        private:
        std::function<void(int, signalfd_siginfo&)> _cb;
        int                                         _fd;
    };

    class epoll_socket : public epoll_handle {
        public:
        epoll_socket(int fd);

        constexpr int get_fd() {
            return _fd;
        }

        void handle(uint32_t events);

        int                               listen(int backlog);
        coro<std::optional<epoll_socket>> accept();

        coro<ssize_t> recv(void* buffer, size_t size);

        private:
        int               _fd;
        await_handle<int> _send_handle;
        await_handle<int> _recv_handle;
    };
}