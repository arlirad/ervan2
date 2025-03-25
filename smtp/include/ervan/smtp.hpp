#pragma once

#include "ervan/coro.hpp"

namespace ervan::smtp {
    coro<int> main_async();
}