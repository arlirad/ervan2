#pragma once

#include "eaio.hpp"
#include "eipc.hpp"

#include <cstdint>

namespace ervan {
    enum func : uint8_t {
        FUNC_RELOADCONFIG,
        FUNC_READCONFIG,
        FUNC_WRITECONFIGKEY,
    };

    eaio::custom wrap_endpoint(eaio::dispatcher& d, eipc::endpoint& ep);
}