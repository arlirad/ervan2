#pragma once

#include <initializer_list>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <tuple>

namespace ervan {
    using config_key_id = uint16_t;

    template <config_key_id ID, typename T>
    struct config_entry {
        static const config_key_id id = ID;
        using type                    = T;
        using shared_ptr              = std::shared_ptr<T>;

        shared_ptr value;

        operator config_key_id() const {
            return this->id;
        }
    };

    struct config_hostname : config_entry<0, std::string> {};
    struct config_port : config_entry<1, uint16_t> {};
    struct config_submissionport : config_entry<2, uint16_t> {};

    template <typename... Args>
    struct config_key_list {
        constexpr config_key_list() {
            this->push<Args...>();
        }

        template <typename T, typename... A>
        constexpr void push(int index = 0) {
            keys[index] = T::id;

            if constexpr (sizeof...(A) > 0)
                push<A...>(index + 1);
        }

        constexpr void push(int index) {}

        const int     count = sizeof...(Args);
        config_key_id keys[sizeof...(Args)];
    };

    template <>
    struct config_key_list<> {
        config_key_list() {}
        config_key_list(const std::initializer_list<config_key_id> args) : count(args.size()) {
            if (this->count > sizeof(keys) / sizeof(config_key_id))
                ::abort();

            int i = 0;

            for (auto key : args)
                keys[i++] = key;
        }

        const int     count = 0;
        config_key_id keys[64];
    };

    template <typename... Keys>
    class config {
        public:
        const config_key_list<Keys...> key_list;

        template <typename K>
        K::type get() {
            const int index = this->find_index<K::id, Keys...>();

            static_assert(index != -1);
            static_assert(index < sizeof...(Keys));

            return *(std::get<index>(_entries).value.get());
        }

        template <typename K>
        void set(K::type val) {
            const int index = this->find_index<K::id, Keys...>();

            static_assert(index != -1);
            static_assert(index < sizeof...(Keys));

            std::get<index>(_entries).value = std::make_shared<typename K::type>(val);
        }

        template <typename K>
        bool ensure() {
            const int index = this->find_index<K::id, Keys...>();

            static_assert(index != -1);
            static_assert(index < sizeof...(Keys));

            return std::get<index>(_entries).value.get();
        }

        private:
        static const int    _count = sizeof...(Keys);
        std::tuple<Keys...> _entries;

        template <config_key_id id, typename T, typename... Args>
        constexpr config_key_id find_index(int index = 0) {
            return id == T::id ? index : find_index<id, Args...>(index + 1);
        }

        template <config_key_id id>
        constexpr config_key_id find_index(int index) {
            return -1;
        }
    };
}