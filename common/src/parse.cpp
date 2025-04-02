#include "ervan/parse.hpp"

namespace ervan::parse {
    bool isatext(int c) {
        return isalnum(c) || std::string("!#$%&'*+-/=?^_`{|}~").find(c) != std::string::npos;
    }

    bool isqtextSMTP(int c) {
        return (c >= 32 && c <= 33) || (c >= 35 && c <= 91) || (c >= 93 && c <= 126);
    }

    bool isestmpvalue(int c) {
        return (c >= 33 && c <= 60) || (c >= 62 && c <= 126);
    }

    result atom(span<const char> _src) {
        auto src = _src;

        if (src.size() == 0 || !isatext(src[0]))
            return malformed_result{};

        src = src.advance(1);

        while (src.size() > 0 && isatext(src[0]))
            src = src.advance(1);

        return {_src, src};
    }

    quoted_string_result quoted_string(span<const char> _src) {
        auto src = _src;
        if (src.size() == 0 || src[0] != '"')
            return malformed_result{};

        src = src.advance(1);

        while (src.size() > 0 && src[0] != '"') {
            if (src[0] == '\\') {
                src = src.advance(1);

                if (src[0] < 32 || src[0] > 126)
                    return malformed_result{};
            }
            else {
                if (!isqtextSMTP(src[0]))
                    return malformed_result{};
            }

            src = src.advance(1);
        }

        if (src.size() == 0 || src[0] != '"')
            return malformed_result{};

        src = src.advance(1);

        return {_src, src};
    }

    local_part_result local_part(span<const char> _src) {
        auto src = _src;

        auto quoted_string_result = quoted_string(src);
        if (quoted_string_result)
            return {quoted_string_result};

        for (;;) {
            auto result = atom(src);
            if (!result)
                return malformed_result{};

            src = result.rest;

            if (src.size() == 0 || src[0] != '.')
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

            if (size < src.size() && src[size] == '-')
                return malformed_result{};

            if (size == src.size() || src[size] != '.')
                break;

            required = true;
            size++;
        }

        return {src, size};
    }

    domain_result at_domain(span<const char> src) {
        if (src.size() == 0 || src[0] != '@')
            return malformed_result{};

        src = src.advance(1);

        return domain(src);
    }

    result a_d_l(span<const char> src) {
        auto at_domain_result = at_domain(src);

        if (at_domain_result) {
            src = at_domain_result.rest;

            while (src.size() > 0 && src[0] == ',') {
                src = src.advance(1);

                at_domain_result = at_domain(src);
                if (!at_domain_result)
                    return malformed_result{};

                src = at_domain_result.rest;
            }

            if (src.size() == 0 || src[0] != ':')
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

        if (src.size() == 0 || src[0] != '@')
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
        if (src.size() == 0 || src[0] != '<')
            return malformed_result{};

        src = src.advance(1);

        auto result = mailbox(src);
        if (!result)
            return malformed_result{};

        src = result.rest;

        if (src.size() == 0 || src[0] != '>')
            return malformed_result{};

        src = src.advance(1);

        return {_src, src, result};
    }

    part_result reverse_path(span<const char> _src) {
        auto src = _src;
        if (src.size() == 0 || src[0] != '<')
            return malformed_result{};

        src = src.advance(1);

        if (src.size() == 0)
            return malformed_result{};

        if (src[0] == '>') {
            src = src.advance(1);
            return {_src, src};
        }

        auto result = mailbox(src);
        if (!result)
            return malformed_result{};

        src = result.rest;

        if (src.size() == 0 || src[0] != '>')
            return malformed_result{};

        src = src.advance(1);

        return {_src, src, result};
    }

    result esmtp_keyword(span<const char> src) {
        if (src.size() == 0 || !isalnum(src[0]))
            return malformed_result{};

        size_t size = 1;

        while (size < src.size() && (isalnum(src[size]) || src[size] == '-'))
            size++;

        return {src, src.advance(size)};
    }

    result esmtp_value(span<const char> src) {
        if (src.size() == 0 || !isestmpvalue(src[0]))
            return malformed_result{};

        size_t size = 1;

        while (size < src.size() && isestmpvalue(src[size]))
            size++;

        return {src, src.advance(size)};
    }

    esmtp_param_result esmtp_param(span<const char> _src) {
        auto src = _src;
        auto key = esmtp_keyword(src);
        if (!key)
            return malformed_result{};

        src = key.rest;

        if (src.size() == 0 || src[0] != '=')
            return malformed_result{};

        src = src.advance(1);

        auto value = esmtp_value(src);
        if (!value)
            return malformed_result{};

        src = value.rest;

        return {_src, src, key, value};
    }
}