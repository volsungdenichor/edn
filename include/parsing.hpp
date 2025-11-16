#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace parsing
{

struct location_t
{
    std::size_t row;
    std::size_t col;

    friend bool operator==(const location_t& lhs, const location_t& rhs)
    {
        return std::tie(lhs.row, lhs.col) == std::tie(rhs.row, rhs.col);
    }

    friend bool operator!=(const location_t& lhs, const location_t& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const location_t& lhs, const location_t& rhs)
    {
        return std::tie(lhs.row, lhs.col) < std::tie(rhs.row, rhs.col);
    }

    friend bool operator>(const location_t& lhs, const location_t& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const location_t& lhs, const location_t& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator>=(const location_t& lhs, const location_t& rhs)
    {
        return !(lhs < rhs);
    }

    friend std::ostream& operator<<(std::ostream& os, const location_t& item)
    {
        return os << item.row << "/" << item.col;
    }
};

struct symbol_t
{
    char value;
    location_t location;

    friend std::ostream& operator<<(std::ostream& os, const symbol_t& item)
    {
        return os << item.value << " (" << item.location << ")";
    }
};

struct stream_t
{
    std::string_view m_content;
    std::size_t m_pos;
    location_t m_loc;

    stream_t(std::string_view content, std::size_t pos, location_t loc) : m_content(content), m_pos(pos), m_loc(loc)
    {
    }

    stream_t(std::string_view content) : stream_t(content, 0, location_t{ 0, 0 })
    {
    }

    location_t location() const
    {
        return m_loc;
    }

    auto reset() const -> stream_t
    {
        return stream_t{ m_content };
    }

    auto get() const -> std::string_view
    {
        return m_content.substr(m_pos);
    }

    explicit operator bool() const
    {
        return !get().empty();
    }

    auto read() const -> std::optional<std::pair<symbol_t, stream_t>>
    {
        const auto content = get();
        if (content.empty())
        {
            return {};
        }
        if (content.size() >= 2 && content[0] == '\r' && content[1] == '\n')
        {
            return { { symbol_t{ '\n', m_loc }, stream_t{ m_content, m_pos + 2, location_t{ m_loc.row + 1, 0 } } } };
        }

        if (content.size() >= 1 && content[0] == '\n')
        {
            return { { symbol_t{ '\n', m_loc }, stream_t{ m_content, m_pos + 1, location_t{ m_loc.row + 1, 0 } } } };
        }

        return { { symbol_t{ content[0], m_loc },
                   stream_t{ m_content, m_pos + 1, location_t{ m_loc.row, m_loc.col + 1 } } } };
    }

    friend std::ostream& operator<<(std::ostream& os, const stream_t& item)
    {
        return os << item.get();
    }
};

struct token_t
{
    std::string value;
    location_t location;

    friend std::ostream& operator<<(std::ostream& os, const token_t& item)
    {
        return os << item.value << " (" << item.location << ")";
    }
};

using parse_result_t = std::pair<token_t, stream_t>;

using maybe_parse_result_t = std::optional<parse_result_t>;

struct parser_t : public std::function<maybe_parse_result_t(stream_t)>
{
    using base_t = std::function<maybe_parse_result_t(stream_t)>;
    using base_t::base_t;
};

struct parser_combinator_t : public std::function<parser_t(parser_t)>
{
    using base_t = std::function<parser_t(parser_t)>;
    using base_t::base_t;
};

using char_predicate_t = std::function<bool(char)>;
using count_predicate_t = std::function<bool(std::size_t)>;

constexpr inline struct slice_fn
{
    struct impl_t
    {
        std::optional<std::ptrdiff_t> begin;
        std::optional<std::ptrdiff_t> end;

        static auto adjust(std::ptrdiff_t index, std::ptrdiff_t size) -> std::ptrdiff_t
        {
            return std::clamp<std::ptrdiff_t>(index >= 0 ? index : index + size, 0, size);
        };

        constexpr auto calculate(std::ptrdiff_t s) const -> std::pair<std::ptrdiff_t, std::ptrdiff_t>
        {
            const std::ptrdiff_t b = begin ? adjust(*begin, s) : std::ptrdiff_t{ 0 };
            const std::ptrdiff_t e = end ? adjust(*end, s) : s;
            return { b, std::max(std::ptrdiff_t{ 0 }, e - b) };
        }

        friend auto operator|(std::string_view str, const impl_t& self) -> std::string_view
        {
            const auto [start, size] = self.calculate(str.size());
            return str.substr(start, size);
        }

        friend auto operator|(std::string str, const impl_t& self) -> std::string
        {
            const auto [start, size] = self.calculate(str.size());
            return str.substr(start, size);
        }
    };

    constexpr auto operator()(std::optional<std::ptrdiff_t> begin, std::optional<std::ptrdiff_t> end) const -> impl_t
    {
        return impl_t{ begin, end };
    };
} slice{};

inline auto eq(char v) -> char_predicate_t
{
    return [=](char ch) { return ch == v; };
}

inline auto ne(char v) -> char_predicate_t
{
    return [=](char ch) { return ch != v; };
}

inline auto one_of(std::string chars) -> char_predicate_t
{
    return [=](char ch) { return chars.find(ch) != std::string::npos; };
}

const inline char_predicate_t is_space = [](char ch) { return std::isspace(ch); };
const inline char_predicate_t is_digit = [](char ch) { return std::isdigit(ch); };
const inline char_predicate_t is_alnum = [](char ch) { return std::isalnum(ch); };
const inline char_predicate_t is_alpha = [](char ch) { return std::isalpha(ch); };
const inline char_predicate_t is_upper = [](char ch) { return std::isupper(ch); };
const inline char_predicate_t is_lower = [](char ch) { return std::islower(ch); };
const inline char_predicate_t is_graph = [](char ch) { return std::isgraph(ch); };

constexpr inline struct character_fn
{
    auto operator()(char_predicate_t pred) const -> parser_t
    {
        return [=](stream_t stream) -> maybe_parse_result_t
        {
            if (auto res = stream.read())
            {
                if (pred(res->first.value))
                {
                    return { { token_t{ std::string(1, res->first.value), res->first.location }, res->second } };
                }
            }
            return {};
        };
    }

    auto operator()(char ch) const -> parser_t
    {
        return (*this)(eq(ch));
    }
} character{};

static const inline auto space = character(is_space);
static const inline auto digit = character(is_digit);
static const inline auto alnum = character(is_alnum);
static const inline auto alpha = character(is_alpha);
static const inline auto upper = character(is_upper);
static const inline auto lower = character(is_lower);
static const inline auto graph = character(is_graph);
constexpr inline struct any_fn
{
    auto operator()(std::vector<parser_t> parsers) const -> parser_t
    {
        return [=](stream_t stream) -> maybe_parse_result_t
        {
            for (const parser_t& parser : parsers)
            {
                if (const auto res = parser(stream))
                {
                    return *res;
                }
            }
            return {};
        };
    }

    template <class... Tail>
    auto operator()(parser_t head, Tail... tail) const -> parser_t
    {
        return (*this)(std::vector<parser_t>{ std::move(head), std::move(tail)... });
    }
} any{};

constexpr inline struct sequence_fn
{
    auto operator()(std::vector<parser_t> parsers) const -> parser_t
    {
        return [=](stream_t stream) -> maybe_parse_result_t
        {
            location_t location = stream.location();
            std::string result = {};
            stream_t remainder = stream;
            for (const parser_t& parser : parsers)
            {
                if (const auto res = parser(remainder))
                {
                    result += res->first.value;
                    remainder = res->second;
                }
                else
                {
                    return {};
                }
            }
            return { { token_t{ std::move(result), location }, remainder } };
        };
    }

    template <class... Tail>
    auto operator()(parser_t head, Tail... tail) const -> parser_t
    {
        return (*this)(std::vector<parser_t>{ std::move(head), std::move(tail)... });
    }
} sequence{};

constexpr inline struct repeat_fn
{
    auto operator()(count_predicate_t pred) const -> parser_combinator_t
    {
        return [=](parser_t parser) -> parser_t
        {
            return [=](stream_t stream) -> maybe_parse_result_t
            {
                location_t location = stream.location();
                std::string result = {};
                std::size_t count = 0;
                stream_t remainder = stream;
                while (remainder)
                {
                    if (const auto res = parser(remainder))
                    {
                        result += res->first.value;
                        remainder = res->second;
                        count += 1;
                    }
                    else
                    {
                        break;
                    }
                }
                if (pred(count))
                {
                    return { { token_t{ std::move(result), location }, remainder } };
                }
                return {};
            };
        };
    }
} repeat{};

const inline parser_combinator_t many = repeat([](std::size_t v) { return v >= 0; });

inline auto at_least(std::size_t count)
{
    return repeat([=](std::size_t v) { return v >= count; });
}

inline auto at_most(std::size_t count) -> parser_combinator_t
{
    return repeat([=](std::size_t v) { return v <= count; });
}

inline auto times(std::size_t lo, std::size_t up) -> parser_combinator_t
{
    return repeat([=](std::size_t v) { return lo <= v && v <= up; });
}

inline auto times(std::size_t count) -> parser_combinator_t
{
    return repeat([=](std::size_t v) { return v == count; });
}

constexpr inline struct optional_fn
{
    auto operator()(parser_t parser) const -> parser_t
    {
        return [=](stream_t stream) -> maybe_parse_result_t
        {
            if (auto res = parser(stream))
            {
                return *res;
            }
            return { { token_t{ "", stream.location() }, stream } };
        };
    }
} optional{};

constexpr inline struct transform_fn
{
    auto operator()(std::function<std::string(std::string)> func) const -> parser_combinator_t
    {
        return [=](parser_t parser) -> parser_t
        {
            return [=](stream_t stream) -> maybe_parse_result_t
            {
                if (auto res = parser(stream))
                {
                    return { { token_t{ func(std::move(res->first.value)), res->first.location }, res->second } };
                }
                return std::nullopt;
            };
        };
    }
} transform{};

constexpr inline struct then_fn
{
    auto operator()(parser_t second) const -> parser_combinator_t
    {
        return [=](parser_t first) -> parser_t
        {
            return [=](stream_t stream) -> maybe_parse_result_t
            {
                if (auto fst = first(stream))
                {
                    return second(fst->second);
                }
                return {};
            };
        };
    }
} then{};

constexpr inline struct skip_fn
{
    auto operator()(parser_t second) const -> parser_combinator_t
    {
        return [=](parser_t first) -> parser_t
        {
            return [=](stream_t stream) -> maybe_parse_result_t
            {
                if (auto fst = first(stream))
                {
                    token_t result = fst->first;
                    if (auto sec = second(fst->second))
                    {
                        return { { std::move(result), sec->second } };
                    }
                }
                return {};
            };
        };
    }
} skip{};

inline auto operator>>(parser_t lhs, parser_t rhs) -> parser_t
{
    return then(std::move(rhs))(std::move(lhs));
}

inline auto operator<<(parser_t lhs, parser_t rhs) -> parser_t
{
    return skip(std::move(rhs))(std::move(lhs));
}

inline auto quoted_string() -> parser_t
{    
    return character(eq('"')) >> many(any(character(eq('\\')) >> character(eq('\"')), character(ne('"'))))
                                     << character(eq('"'));
}

constexpr inline struct tokenize_fn
{
    template <class State, class Func>
    auto operator()(stream_t stream, const parser_t& parser, State state, Func func) const -> State
    {
        while (stream)
        {
            if (const auto res = parser(stream))
            {
                state = std::invoke(func, std::move(state), std::move(res->first));
                stream = res->second;
            }
            else
            {
                break;
            }
        }
        return state;
    }

    template <class Out>
    auto operator()(stream_t stream, const parser_t& parser, Out out) const -> Out
    {
        const auto func = [](Out o, token_t token) -> Out
        {
            *o++ = std::move(token);
            return o;
        };
        return (*this)(stream, parser, std::move(out), func);
    }

    auto operator()(stream_t stream, const parser_t& parser) const -> std::vector<token_t>
    {
        std::vector<token_t> out;
        (*this)(stream, parser, std::back_inserter(out));
        return out;
    }
} tokenize{};

}  // namespace parsing
