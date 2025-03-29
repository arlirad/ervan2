#include "ervan/ipc.hpp"

namespace ervan {
    eaio::custom wrap_endpoint(eaio::dispatcher& d, eipc::endpoint& ep) {
        return d.wrap(ep.native_handle, [&ep](uint32_t ev) {
            if (ev & eaio::FLAG_IN)
                while (ep.try_receive())
                    ;
        });
    }

}