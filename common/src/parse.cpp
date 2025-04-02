#include "ervan/parse.hpp"

namespace ervan::parse {
    bool isatext(int c) {
        return isalnum(c) || std::string("!#$%&'*+-/=?^_`{|}~").find(c) != std::string::npos;
    }

    result atom(span<const char> _src) {
        auto src = _src;

        if (!isatext(src[0]))
            return malformed_result{};

        src = src.advance(1);

        while (isatext(src[0]))
            src = src.advance(1);

        return {_src, src};
    }

    local_part_result local_part(span<const char> _src) {
        auto src = _src;

        for (;;) {
            auto result = atom(src);
            if (!result)
                return malformed_result{};

            src = result.rest;

            if (src[0] != '.')
                break;

            src = src.advance(1);
        }

        return {_src, src};
    }

    domain_result domain(span<const char> src) {
        size_t size     = 0;
        bool   required = true;

        for (;;) {
            for (; size < src.size(); size++) {
                if (!std::isalnum(src[size]) && src[size] != '-') {
                    if (required)
                        return malformed_result{};

                    break;
                }

                required = false;
            }

            if (src[size] == '-')
                return malformed_result{};

            if (src[size] != '.')
                break;

            required = true;
            size++;
        }

        return {src, size};
    }

    domain_result at_domain(span<const char> src) {
        if (src[0] != '@')
            return malformed_result{};

        src = src.advance(1);

        return domain(src);
    }

    result a_d_l(span<const char> src) {
        auto at_domain_result = at_domain(src);

        if (at_domain_result) {
            src = at_domain_result.rest;

            while (src[0] == ',') {
                src = src.advance(1);

                at_domain_result = at_domain(src);
                if (!at_domain_result)
                    return malformed_result{};

                src = at_domain_result.rest;
            }

            if (src[0] != ':')
                return malformed_result{};

            src = src.advance(1);
        }

        return {src};
    }

    mailbox_result mailbox(span<const char> _src) {
        auto src = _src;

        auto a_d_l_result = a_d_l(src);
        src               = a_d_l_result.rest;

        auto local_part_result = local_part(src);
        if (!local_part_result)
            return malformed_result{};

        src = local_part_result.rest;

        if (src[0] != '@')
            return malformed_result{};

        src = src.advance(1);

        auto domain_result = domain(src);
        if (!domain_result)
            return malformed_result{};

        src = domain_result.rest;
        return {_src, src, local_part_result, domain_result};
    }

    part_result path(span<const char> _src) {
        auto src = _src;
        if (src[0] != '<')
            return malformed_result{};

        src = src.advance(1);

        auto result = mailbox(src);
        if (!result)
            return malformed_result{};

        src = result.rest;

        if (src[0] != '>')
            return malformed_result{};

        src = src.advance(1);

        return {_src, src, result};
    }

    part_result reverse_path(span<const char> _src) {
        auto src = _src;
        if (src[0] != '<')
            return malformed_result{};

        src = src.advance(1);

        if (src[0] == '>') {
            src = src.advance(1);
            return {_src, src};
        }

        auto result = mailbox(src);
        if (!result)
            return malformed_result{};

        src = result.rest;

        if (src[0] != '>')
            return malformed_result{};

        src = src.advance(1);

        return {_src, src, result};
    }
}