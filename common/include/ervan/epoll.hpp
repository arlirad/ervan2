#pragma once

#include "eipc.hpp"
#include <sys/epoll.h>
#include <sys/signalfd.h>

namespace ervan {
    class epoll_handle {
        public:
        virtual int  get_fd() = 0;
        virtual void handle() = 0;
    };

    class epoll_endpoint : public epoll_handle {
        public:
        epoll_endpoint(eipc::endpoint& ep) : _ep(ep) {}

        constexpr int get_fd() {
            return this->_ep.native_handle;
        }

        void handle() {
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

        void handle();

        private:
        std::function<void(int, signalfd_siginfo&)> _cb;
        int                                         _fd;
    };

    struct epoll_handler {
        public:
        static const int MAX_EVENTS = 32;

        epoll_handler();

        void add(epoll_handle* handle);
        void poll();

        private:
        int                        _fd;
        epoll_event                _events[MAX_EVENTS];
        std::vector<epoll_handle*> _handles;
    };
}