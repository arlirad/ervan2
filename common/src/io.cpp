#include "ervan/io.hpp"

#include <aio.h>
#include <fcntl.h>
#include <signal.h>

namespace ervan::io {
    void io_signal(int, siginfo_t*, void*);

    void install_io_signal() {
        struct sigaction action = {};

        action.sa_sigaction = io_signal;
        action.sa_flags     = SA_SIGINFO;
        sigemptyset(&action.sa_mask);

        ::sigaction(SIGIO, &action, nullptr);
    }

    eaio::coro<int> sync_file(eaio::file& file) {
        eaio::await_handle<void> handle;
        aiocb                    aiocb = {.aio_fildes = file.native_handle(), .aio_sigevent = {}};

        aiocb.aio_sigevent.sigev_value.sival_ptr = &handle;
        aiocb.aio_sigevent.sigev_signo           = SIGIO;
        aiocb.aio_sigevent.sigev_notify          = SIGEV_SIGNAL;

        int result = ::aio_fsync(O_DSYNC, &aiocb);
        if (result != 0)
            co_return result;

        co_await handle;
        co_return 0;
    }

    void io_signal(int signum, siginfo_t* info, void* ctx) {
        auto handle = reinterpret_cast<eaio::await_handle<void>*>(info->si_ptr);

        handle->notify();
    }
}