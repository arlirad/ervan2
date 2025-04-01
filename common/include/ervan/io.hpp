#pragma once

#include <eaio.hpp>

namespace ervan::io {
    void            install_io_signal();
    eaio::coro<int> sync_file(eaio::file& file);
}