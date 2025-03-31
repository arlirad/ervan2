#pragma once

#include "eaio.hpp"
#include "ervan/config.hpp"
#include "ervan/smtp/session.hpp"

namespace ervan::smtp {
    using local_config =
        config<config_hostname, config_port, config_submissionport, config_maxmessagesize>;
    extern local_config cfg;

    eaio::coro<void> handle(eaio::socket sock);
    eaio::coro<int>  main_async();
}