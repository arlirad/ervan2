#pragma once

#include "eaio.hpp"
#include "ervan/config.hpp"
#include "ervan/smtp/session.hpp"
#include "ervan/smtp/types.hpp"
#include "ervan/string.hpp"

namespace ervan::smtp {
    using local_config =
        config<config_hostname, config_port, config_submissionport, config_maxmessagesize>;
    extern local_config cfg;

    eaio::coro<int> main_async();
}