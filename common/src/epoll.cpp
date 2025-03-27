#include "ervan/epoll.hpp"

namespace ervan {
    epoll_handler::epoll_handler() {
        this->_fd = epoll_create1(0);
    }

    void epoll_handler::add(epoll_handle* handle) {
        epoll_event ev;

        ev.events   = EPOLLIN;
        ev.data.fd  = handle->get_fd();
        ev.data.ptr = reinterpret_cast<void*>(handle);

        epoll_ctl(this->_fd, EPOLL_CTL_ADD, handle->get_fd(), &ev);
    }

    void epoll_handler::remove(epoll_handle* handle) {
        epoll_event ev;

        ev.events  = EPOLLIN | EPOLLOUT | EPOLLERR;
        ev.data.fd = handle->get_fd();

        epoll_ctl(this->_fd, EPOLL_CTL_DEL, handle->get_fd(), &ev);
    }

    void epoll_handler::poll() {
        int nfds = epoll_wait(this->_fd, this->_events, this->MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++) {
            epoll_event& event = this->_events[i];

            // what could possibly go wrong, right?
            auto handle = reinterpret_cast<epoll_handle*>(event.data.ptr);

            handle->handle(event.events);

            /*for (int j = 0; j < this->_handles.size(); j++) {
                if (event.data.fd != this->_handles[j]->get_fd())
                    continue;

                this->_handles[j]->handle(event.events);
                break;
            }*/
        }

        for (auto handle : this->to_cleanup)
            handle.destroy();

        this->to_cleanup.clear();
    }
}