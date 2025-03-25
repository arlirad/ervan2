#include <sys/epoll.h>

#include <eipc.hpp>
#include <vector>

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

    struct epoll_context {
        static const int MAX_EVENTS = 32;

        int                        fd;
        epoll_event                events[MAX_EVENTS];
        std::vector<epoll_handle*> handles;
    };

    epoll_context epoll_setup() {
        return {.fd = epoll_create1(0), .events = {}};
    }

    void epoll_add(epoll_context& ctx, epoll_handle* handle) {
        epoll_event ev;

        ev.events  = EPOLLIN;
        ev.data.fd = handle->get_fd();

        epoll_ctl(ctx.fd, EPOLL_CTL_ADD, handle->get_fd(), &ev);
        ctx.handles.push_back(handle);
    }

    void epoll_poll(epoll_context& ctx) {
        int nfds = epoll_wait(ctx.fd, ctx.events, ctx.MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++) {
            epoll_event& event = ctx.events[i];

            for (int j = 0; j < ctx.handles.size(); j++) {
                if (event.data.fd != ctx.handles[j]->get_fd())
                    continue;

                ctx.handles[j]->handle();
                break;
            }
        }
    }
}