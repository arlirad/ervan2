#include "ervan/epoll.hpp"

#include <signal.h>


namespace ervan {
    epoll_signal::epoll_signal(int sig_id, std::function<void(int, signalfd_siginfo&)> cb)
        : _cb(cb) {
        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, sig_id);

        this->_fd = signalfd(-1, &sigset, SFD_NONBLOCK);
    }

    void epoll_signal::handle() {
        signalfd_siginfo siginfo;

        if (read(this->_fd, &siginfo, sizeof(siginfo)) == -1)
            return;

        this->_cb(siginfo.ssi_signo, siginfo);
    }
}