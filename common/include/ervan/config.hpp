#pragma once

#include <initializer_list>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <tuple>

namespace ervan {
    constexpr auto CONFIG_PATH = "./config";

    template <typename... Keys>
    class config;

    using config_key_id = uint16_t;

    template <size_t N>
    struct config_name {
        char name[N]{};

        constexpr config_name(const char (&str)[N]) {
            std::copy_n(str, N, name);
        }
    };

    template <config_key_id ID, typename T, config_name N>
    struct config_entry {
        static const config_key_id   id   = ID;
        static constexpr config_name name = N;

        using type       = T;
        using shared_ptr = std::shared_ptr<T>;

        shared_ptr value;

        operator config_key_id() const {
            return this->id;
        }
    };

    struct config_hostname : config_entry<0, std::string, "hostname"> {};
    struct config_port : config_entry<1, uint16_t, "port"> {};
    struct config_submissionport : config_entry<2, uint16_t, "submissionport"> {};
    struct config_debug : config_entry<3, bool, "debug"> {};

    using config_all = config<config_hostname, config_port, config_submissionport, config_debug>;

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

        bool dump(int id, char* buffer, size_t len) {
            return this->dump_by_id<void, Keys...>(id, buffer, len);
        }

        template <typename K>
        void set(K::type val) {
            const int index = this->find_index<K::id, Keys...>();

            static_assert(index != -1);
            static_assert(index < sizeof...(Keys));

            std::get<index>(_entries).value = std::make_shared<typename K::type>(val);
        }

        bool set(std::string& key, std::string& val) {
            return this->set_by_key<void, Keys...>(key, val);
        }

        bool set(int id, const char* buffer, size_t len) {
            return this->set_by_id<void, Keys...>(id, buffer, len);
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

        template <typename>
        constexpr bool dump_by_id(config_key_id id, char* buffer, size_t len) {
            return false;
        }

        template <typename, typename T, typename... Args>
        constexpr bool dump_by_id(config_key_id id, char* buffer, size_t len) {
            return id == T::id ? this->dump_by_id_found<T>(buffer, len)
                               : this->dump_by_id<void, Args...>(id, buffer, len);
        }

        template <typename>
        constexpr bool set_by_key(std::string&, std::string&) {
            return false;
        }

        template <typename, typename T, typename... Args>
        constexpr bool set_by_key(std::string& key, std::string& val) {
            return this->compare<T>(key) ? this->transform_set<T>(val)
                                         : this->set_by_key<void, Args...>(key, val);
        }

        template <typename>
        constexpr bool set_by_id(int id, const char* buffer, size_t len) {
            return false;
        }

        template <typename, typename T, typename... Args>
        constexpr bool set_by_id(int id, const char* buffer, size_t len) {
            return id == T::id ? this->set_by_id_found<T>(buffer, len)
                               : this->set_by_id<void, Args...>(id, buffer, len);
        }

        template <config_key_id id>
        constexpr int find_index(int index) {
            return -1;
        }

        template <config_key_id id, typename T, typename... Args>
        constexpr int find_index(int index = 0) {
            return id == T::id ? index : this->find_index<id, Args...>(index + 1);
        }

        template <typename K>
        constexpr bool dump_by_id_found(char* buffer, size_t len) {
            const int index = this->find_index<K::id, Keys...>();

            if (!std::get<index>(_entries).value)
                return false;

            return dump_write(std::get<index>(_entries).value.get(), buffer, len);
        }

        constexpr bool dump_write(std::string* val, char* buffer, size_t len) {
            if (len < val->size() + 1)
                return false;

            strncpy(buffer, val->c_str(), len);
            return true;
        }

        template <typename T>
        constexpr bool dump_write(T* val, char* buffer, size_t len) {
            if (len < sizeof(T))
                return false;

            *(reinterpret_cast<T*>(buffer)) = *val;
            return true;
        }

        template <typename K>
        constexpr bool transform_set(std::string& val) {
            const int index = this->find_index<K::id, Keys...>();

            static_assert(index != -1);
            static_assert(index < sizeof...(Keys));

            auto transform_try = this->transform(std::get<index>(_entries).value, val);
            if (!transform_try)
                return false;

            auto value = transform_try.value();

            std::get<index>(_entries).value = std::make_shared<typename K::type>(value);

            return true;
        }

        constexpr std::optional<std::string> transform(std::shared_ptr<std::string>&,
                                                       std::string& str) {
            return str;
        }

        constexpr std::optional<uint16_t> transform(std::shared_ptr<uint16_t>&, std::string& str) {
            uint32_t value;
            auto     result = std::from_chars(str.c_str(), str.c_str() + str.size(), value);

            if (result.ec == std::errc::invalid_argument ||
                value > std::numeric_limits<uint16_t>::max())
                return {};

            return value;
        }

        constexpr std::optional<bool> transform(std::shared_ptr<bool>&, std::string& str) {
            return str == "yes" || str == "true";
        }

        template <typename K>
        constexpr bool set_by_id_found(const char* buffer, size_t len) {
            const int index = this->find_index<K::id, Keys...>();

            return set_write(std::get<index>(_entries).value, buffer, len);
        }

        constexpr bool set_write(std::shared_ptr<std::string>& ptr, const char* buffer,
                                 size_t len) {
            ptr = std::make_shared<std::string>(buffer);
            return true;
        }

        template <typename T>
        constexpr bool set_write(std::shared_ptr<T>& ptr, const char* buffer, size_t len) {
            if (len < sizeof(T))
                return false;

            ptr = std::make_shared<T>(*(reinterpret_cast<const T*>(buffer)));
            return true;
        }

        template <typename K>
        constexpr bool compare(const std::string& compared_to) {
            return strcmp(K::name.name, compared_to.c_str()) == 0;
        }
    };
}