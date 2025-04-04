#pragma once

#include <eaio.hpp>
#include <string>
#include <vector>

namespace ervan {
    struct metadata {
        char line_a[32] = {};
        char line_b[32] = {};
        char line_c[32] = {};
        char line_d[32] = {};

        std::string              reverse_path;
        std::vector<std::string> forward_paths;

        eaio::coro<bool> write_to(eaio::file& file);
    };
}