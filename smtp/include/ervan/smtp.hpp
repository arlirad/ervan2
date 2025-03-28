#pragma once

#include "eaio.hpp"

namespace ervan::smtp {
    eaio::coro<int> main_async();
}