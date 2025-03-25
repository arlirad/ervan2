#include "ervan/epoll.hpp"

namespace ervan {
    epoll_handler::epoll_handler() {
        this->_fd = epoll_create1(0);
    }

    void epoll_handler::add(epoll_handle* handle) {
        epoll_event ev;

        ev.events  = EPOLLIN;
        ev.data.fd = handle->get_fd();

        epoll_ctl(this->_fd, EPOLL_CTL_ADD, handle->get_fd(), &ev);
        this->_handles.push_back(handle);
    }

    void epoll_handler::poll() {
        int nfds = epoll_wait(this->_fd, this->_events, this->MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++) {
            epoll_event& event = this->_events[i];

            for (int j = 0; j < this->_handles.size(); j++) {
                if (event.data.fd != this->_handles[j]->get_fd())
                    continue;

                this->_handles[j]->handle();
                break;
            }
        }
    }
}