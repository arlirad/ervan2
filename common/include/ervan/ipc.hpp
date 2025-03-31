#pragma once

#include "eaio.hpp"
#include "eipc.hpp"
#include "ervan/config.hpp"

#include <cstdint>

namespace ervan {
    enum func : uint8_t {
        FUNC_RELOADCONFIG,
        FUNC_READCONFIG,
        FUNC_WRITECONFIGKEY,
    };

    struct config_update {
        config_key_id key;
        char          data[256];
    };

    eaio::custom wrap_endpoint(eaio::dispatcher& d, eipc::endpoint& ep);
}