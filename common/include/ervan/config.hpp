#pragma once

#include <initializer_list>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>

namespace ervan {
    using config_key_id = uint16_t;

    template <config_key_id ID, typename T>
    struct config_key {
        static const config_key_id id = ID;
        using type                    = T;

        operator config_key_id() const {
            return this->id;
        }
    };

    struct config_hostname : config_key<0, std::string> {};
    struct config_port : config_key<1, uint16_t> {};
    struct config_submissionport : config_key<2, uint16_t> {};

    template <typename... K>
    class config {
        public:
        template <typename T>
        T get(config_key_id key);

        template <typename T>
        void set(config_key_id key, T val);

        private:
    };

    struct config_key_list {
        config_key_list() {}
        config_key_list(const std::initializer_list<config_key_id> args) {
            this->count = args.size();

            if (this->count > sizeof(keys) / sizeof(config_key_id))
                ::abort();

            int i = 0;

            for (auto key : args)
                keys[i++] = key;
        }

        int           count = 0;
        config_key_id keys[64];
    };
}