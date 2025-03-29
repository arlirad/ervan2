#pragma once

#include "eaio.hpp"
#include "ervan/config.hpp"

namespace ervan::smtp {
    using local_config = config<config_hostname, config_port, config_submissionport>;

    eaio::coro<int> main_async();
}