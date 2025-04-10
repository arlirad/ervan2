#include <ctime>
#include <eaio.hpp>
#include <unistd.h>

namespace ervan::rfc5322 {
    const size_t preffered_line_length = 78;
    const size_t max_line_length       = 998;

    struct writer {
        eaio::file& base;
        int         line_length;

        writer(eaio::file& file);

        eaio::coro<bool> begin_header(const char* name);
        eaio::coro<bool> write_body_part(const char* part, bool spaced = true);
        eaio::coro<bool> write_time(tm* t);
        eaio::coro<bool> end_header();
    };
};