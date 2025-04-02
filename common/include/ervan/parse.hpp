#pragma once

#include "ervan/string.hpp"

#include <cstring>

namespace ervan::parse {
    const size_t null_space          = sizeof('\0');
    const size_t local_part_max_size = 64;
    const size_t domain_max_size     = 255;
    const size_t mailbox_max_size    = local_part_max_size + sizeof('@') + domain_max_size;

    using local_part_buffer = char[local_part_max_size + null_space];
    using domain_buffer     = char[domain_max_size + null_space];

    struct malformed_result {};

    struct result {
        span<const char> body;
        span<const char> rest;
        bool             malformed = false;

        result() {}

        template <size_t N>
        result(span<const char> src, span<const char> _rest, size_t size, char (&buffer)[N]) {
            if (size >= N) {
                this->malformed = true;
                return;
            }

            this->body = span<const char>(src.begin(), size);
            this->rest = _rest;

            std::memcpy(buffer, src.begin(), size);
            buffer[size] = '\0';
        }

        result(span<const char> _rest) {
            this->rest = _rest;
        }

        result(span<const char> _base, span<const char> _rest) {
            this->body = span(_base.begin(), _rest.begin());
            this->rest = _rest;
        }

        result(const malformed_result) {
            this->malformed = true;
        }

        operator bool() const {
            return !this->malformed;
        }
    };

    struct path_result : public result {
        using result::result;
    };

    struct atom_result : public result {
        parse::domain_buffer buffer;

        using result::result;

        atom_result(span<const char> src, span<const char> rest)
            : result(src, src.advance(rest.begin() - src.begin()), rest.begin() - src.begin(),
                     this->buffer) {}
    };

    struct local_part_result : public result {
        parse::local_part_buffer buffer;

        using result::result;

        local_part_result(span<const char> src, span<const char> rest)
            : result(src, src.advance(rest.begin() - src.begin()), rest.begin() - src.begin(),
                     this->buffer) {}
    };

    struct domain_result : public result {
        using result::result;

        domain_result(span<const char> src, size_t size)
            : result(src, src.advance(size), size, this->buffer) {}

        parse::domain_buffer buffer;
    };

    struct mailbox_result : public result {
        using result::result;

        local_part_result local_part;
        domain_result     domain;

        mailbox_result(span<const char> _src, span<const char> _rest,
                       local_part_result& _local_part, domain_result& _domain)
            : result(_src, _rest) {
            this->domain     = _domain;
            this->local_part = _local_part;
        }
    };

    struct part_result : public result {
        using result::result;

        mailbox_result mailbox;

        part_result(span<const char> _src, span<const char> _rest) : result(_src, _rest) {}

        part_result(span<const char> _src, span<const char> _rest, mailbox_result& _mailbox)
            : result(_src, _rest) {
            this->mailbox = _mailbox;
        }
    };

    result            atom(span<const char> src);
    local_part_result local_part(span<const char> src);
    domain_result     domain(span<const char> src);
    domain_result     at_domain(span<const char> src);
    result            a_d_l(span<const char> src);
    mailbox_result    mailbox(span<const char> src);
    part_result       path(span<const char> src);
    part_result       reverse_path(span<const char> src);
}