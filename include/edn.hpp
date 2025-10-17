#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace edn
{

using boolean_t = bool;
using integer_t = std::int32_t;
using floating_point_t = double;
using character_t = char;

struct nil_t;
struct list_t;
struct vector_t;
struct set_t;
struct map_t;
struct string_t;
struct symbol_t;
struct keyword_t;
struct tagged_element_t;

using value_t = std::variant<
    nil_t,
    boolean_t,
    integer_t,
    floating_point_t,
    string_t,
    character_t,
    symbol_t,
    keyword_t,
    list_t,
    vector_t,
    set_t,
    map_t,
    tagged_element_t>;

enum class type_t
{
    nil,
    boolean,
    integer,
    floating_point,
    string,
    character,
    symbol,
    keyword,
    list,
    vector,
    set,
    map,
    tagged_element,
};

inline std::ostream& operator<<(std::ostream& os, const type_t item)
{
    switch (item)
    {
#define CASE(x) \
    case type_t::x: return os << #x
        CASE(nil);
        CASE(boolean);
        CASE(integer);
        CASE(floating_point);
        CASE(string);
        CASE(character);
        CASE(symbol);
        CASE(keyword);
        CASE(list);
        CASE(vector);
        CASE(set);
        CASE(map);
        CASE(tagged_element);
#undef CASE
        default: break;
    }
    return os;
}

namespace detail
{

constexpr inline struct str_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::string
    {
        std::stringstream ss;
        (ss << ... << std::forward<Args>(args));
        return ss.str();
    }
} str;

constexpr inline struct delimit_fn
{
    template <class Iter>
    struct impl_t
    {
        Iter begin;
        Iter end;
        std::string_view delimiter;

        friend std::ostream& operator<<(std::ostream& os, const impl_t& item)
        {
            for (Iter it = item.begin; it != item.end; ++it)
            {
                if (it != item.begin)
                {
                    os << item.delimiter;
                }
                os << *it;
            }
            return os;
        }
    };

    template <class Iter>
    auto operator()(Iter begin, Iter end, std::string_view delimiter) const -> impl_t<Iter>
    {
        return impl_t<Iter>{ begin, end, delimiter };
    }

    template <class Range>
    auto operator()(Range&& range, std::string_view delimiter) const -> impl_t<decltype(std::begin(range))>
    {
        return (*this)(std::begin(range), std::end(range), delimiter);
    }
} delimit;

template <class Range, class Fmt>
static void format_range(std::ostream& os, Range&& range, Fmt&& fmt)
{
    const auto b = std::begin(range);
    const auto e = std::end(range);
    for (auto it = b; it != e; ++it)
    {
        if (it != b)
        {
            os << " ";
        }
        fmt(os, *it);
    }
}

template <class Range>
static void format_range(std::ostream& os, Range&& range)
{
    const auto b = std::begin(range);
    const auto e = std::end(range);
    for (auto it = b; it != e; ++it)
    {
        if (it != b)
        {
            os << " ";
        }
        os << *it;
    }
}

template <class E, class... Args>
auto exception(Args&&... args) -> E
{
    return E{ str(std::forward<Args>(args)...) };
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace detail

using detail::delimit;
using detail::overloaded;
using detail::str;

struct nil_t
{
    friend std::ostream& operator<<(std::ostream& os, const nil_t&)
    {
        return os << "nil";
    }
};

struct string_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const string_t& item)
    {
        return os << std::quoted(item);
    }
};

struct symbol_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;
};

struct keyword_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const keyword_t& item)
    {
        return os << ":" << (const base_t&)item;
    }
};

struct list_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_t& item)
    {
        os << "(";
        detail::format_range(os, item);
        os << ")";
        return os;
    }
};

struct vector_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const vector_t& item)
    {
        os << "[";
        detail::format_range(os, item);
        os << "]";
        return os;
    }
};

struct set_t : public std::set<value_t>
{
    using base_t = std::set<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const set_t& item)
    {
        os << "#{";
        detail::format_range(os, item);
        os << "}";
        return os;
    }
};

struct map_t : public std::map<value_t, value_t>
{
    using base_t = std::map<value_t, value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item)
    {
        os << "{";
        detail::format_range(os, item, [&](std::ostream& s, const auto& it) { s << it.first << " " << it.second; });
        os << "}";
        return os;
    }
};

struct tagged_element_t
{
    symbol_t m_tag;
    std::unique_ptr<value_t> m_element;

    tagged_element_t(symbol_t tag, value_t value)
        : m_tag(std::move(tag))
        , m_element(std::make_unique<value_t>(std::move(value)))
    {
    }

    tagged_element_t(const tagged_element_t& other) : tagged_element_t(other.tag(), other.element())
    {
    }

    tagged_element_t(tagged_element_t&& other) noexcept = default;

    tagged_element_t() : tagged_element_t(symbol_t{}, value_t{})
    {
    }

    const symbol_t& tag() const
    {
        return m_tag;
    }

    const value_t& element() const
    {
        return *m_element;
    }

    friend bool operator==(const tagged_element_t& lhs, const tagged_element_t& rhs)
    {
        return std::tie(lhs.tag(), lhs.element()) == std::tie(rhs.tag(), rhs.element());
    }

    friend bool operator!=(const tagged_element_t& lhs, const tagged_element_t& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const tagged_element_t& lhs, const tagged_element_t& rhs)
    {
        return std::tie(lhs.tag(), lhs.element()) < std::tie(rhs.tag(), rhs.element());
    }

    friend bool operator>(const tagged_element_t& lhs, const tagged_element_t& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const tagged_element_t& lhs, const tagged_element_t& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator>=(const tagged_element_t& lhs, const tagged_element_t& rhs)
    {
        return !(lhs < rhs);
    }
};

inline type_t type(const value_t& value)
{
    return std::visit(
        overloaded{
            [](nil_t) { return type_t::nil; },
            [](boolean_t) { return type_t::boolean; },
            [](character_t) { return type_t::character; },
            [](integer_t) { return type_t::integer; },
            [](floating_point_t) { return type_t::floating_point; },
            [](const string_t&) { return type_t::string; },
            [](const symbol_t&) { return type_t::symbol; },
            [](const keyword_t&) { return type_t::keyword; },
            [](const list_t&) { return type_t::list; },
            [](const vector_t&) { return type_t::vector; },
            [](const set_t&) { return type_t::set; },
            [](const map_t&) { return type_t::map; },
            [](const tagged_element_t&) { return type_t::tagged_element; },
        },
        value);
}

template <class T>
const T* try_as(const value_t& value)
{
    return std::get_if<T>(&value);
}

template <class T>
const T& as(const value_t& value)
{
    if (const T* res = try_as<T>(value))
    {
        return *res;
    }
    throw detail::exception<std::runtime_error>(
        "Invalid type: expected <", "TODO", ">, actual '", value, "' of type <", type(value), ">");
}

static const inline std::vector<std::tuple<char, std::string>> character_names = {
    { ' ', "space" },
    { '\n', "newline" },
    { '\t', "tab" },
};

void format_character(std::ostream& os, character_t v)
{
    for (const auto& [ch, value] : character_names)
    {
        if (ch == v)
        {
            os << '\\' << value;
            return;
        }
    }
    os << '\\' << v;
}

inline std::ostream& operator<<(std::ostream& os, const value_t& item)
{
    std::visit(
        overloaded{
            [&](nil_t v) { os << v; },
            [&](boolean_t v) { os << (v ? "true" : "false"); },
            [&](character_t v) { format_character(os, v); },
            [&](integer_t v) { os << v; },
            [&](floating_point_t v) { os << v; },
            [&](const string_t& v) { os << v; },
            [&](const symbol_t& v) { os << v; },
            [&](const keyword_t& v) { os << v; },
            [&](const list_t& v) { os << v; },
            [&](const vector_t& v) { os << v; },
            [&](const set_t& v) { os << v; },
            [&](const map_t& v) { os << v; },
            [&](const tagged_element_t& v) { os << v; },
        },
        item);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const tagged_element_t& item)
{
    return os << "#" << item.tag() << " " << item.element();
}

inline bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(
        overloaded{ [](nil_t lt, nil_t rt) { return true; },
                    [](boolean_t lt, boolean_t rt) { return lt == rt; },
                    [](character_t lt, character_t rt) { return lt == rt; },
                    [](integer_t lt, integer_t rt) { return lt == rt; },
                    [](floating_point_t lt, floating_point_t rt)
                    { return std::abs(lt - rt) < std::numeric_limits<floating_point_t>::epsilon(); },
                    [](const string_t& lt, const string_t& rt) { return lt == rt; },
                    [](const symbol_t& lt, const symbol_t& rt) { return lt == rt; },
                    [](const keyword_t& lt, const keyword_t& rt) { return lt == rt; },
                    [](const list_t& lt, const list_t& rt) { return lt == rt; },
                    [](const vector_t& lt, const vector_t& rt) { return lt == rt; },
                    [](const set_t& lt, const set_t& rt) { return lt == rt; },
                    [](const map_t& lt, const map_t& rt) { return lt == rt; },
                    [](const tagged_element_t& lt, const tagged_element_t& rt) { return lt == rt; },
                    [](const auto& lt, const auto& rt) { return false; } },
        lhs,
        rhs);
}

inline bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(
        overloaded{ [](nil_t lt, nil_t rt) { return false; },
                    [](boolean_t lt, boolean_t rt) { return lt < rt; },
                    [](character_t lt, character_t rt) { return lt < rt; },
                    [](integer_t lt, integer_t rt) { return lt < rt; },
                    [](floating_point_t lt, floating_point_t rt) { return lt < rt; },
                    [](const string_t& lt, const string_t& rt) { return lt < rt; },
                    [](const symbol_t& lt, const symbol_t& rt) { return lt < rt; },
                    [](const keyword_t& lt, const keyword_t& rt) { return lt < rt; },
                    [](const list_t& lt, const list_t& rt) { return lt < rt; },
                    [](const vector_t& lt, const vector_t& rt) { return lt < rt; },
                    [](const set_t& lt, const set_t& rt) { return lt < rt; },
                    [](const map_t& lt, const map_t& rt) { return lt < rt; },
                    [](const tagged_element_t& lt, const tagged_element_t& rt) { return lt < rt; },
                    [](const auto& lt, const auto& rt) { return false; } },
        lhs,
        rhs);
}

inline bool operator>(const value_t& lhs, const value_t& rhs)
{
    return rhs < lhs;
}

inline bool operator<=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs > rhs);
}

inline bool operator>=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs < rhs);
}

namespace detail
{

using token_t = std::string;

constexpr inline auto is_quotation_mark(char ch) -> bool
{
    return ch == '"';
};

constexpr inline struct tokenize_fn
{
    auto operator()(std::string_view text) const -> std::vector<token_t>
    {
        std::vector<token_t> result;
        while (!text.empty())
        {
            if (const auto res = read_token(text))
            {
                auto [token, remainder] = *res;
                if (!token.empty())
                {
                    result.push_back(std::move(token));
                }
                text = remainder;
            }
            else
            {
                break;
            }
        }
        return result;
    }

private:
    using tokenizer_result_t = std::tuple<token_t, std::string_view>;

    static auto drop(std::string_view str, std::size_t n) -> std::string_view
    {
        str.remove_prefix(std::min(str.size(), n));
        return str;
    }

    static auto drop_while(std::string_view str, const std::function<bool(char)>& pred) -> std::string_view
    {
        while (!str.empty() && pred(str[0]))
        {
            str.remove_prefix(1);
        }
        return str;
    }

    static auto to_string_view(std::string_view::iterator b, std::string_view::iterator e) -> std::string_view
    {
        return b != e ? std::string_view{ &*b, static_cast<std::size_t>(std::distance(b, e)) } : std::string_view{};
    }

    static auto read_quoted_string(std::string_view text) -> std::optional<tokenizer_result_t>
    {
        static const auto quotation_mark = '"';
        if (text.empty() || !is_quotation_mark(text.front()))
        {
            return {};
        }
        text = drop(text, 1);
        token_t result = {};
        while (!text.empty())
        {
            if (text[0] == '\\' && text.size() > 1 && is_quotation_mark(text[1]))
            {
                result += quotation_mark;
                text = drop(text, 2);
            }
            else if (is_quotation_mark(text[0]))
            {
                return tokenizer_result_t{ token_t{ str(quotation_mark, result, quotation_mark) }, drop(text, 1) };
            }
            else
            {
                result += text[0];
                text = drop(text, 1);
            }
        }
        return std::nullopt;
    }

    static auto read_token(std::string_view text) -> std::optional<tokenizer_result_t>
    {
        static const auto is_parenthesis
            = [](char ch) { return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}'; };
        static const auto is_space = [](char ch) { return std::isspace(ch) || ch == ','; };
        static const auto is_comment = [](char ch) { return ch == ';'; };
        static const auto is_new_line = [](char ch) { return ch == '\n'; };

        if (text.empty())
        {
            return std::nullopt;
        }
        if (is_comment(text[0]))
        {
            return read_token(drop_while(text, std::not_fn(is_new_line)));
        }
        if (text[0] == '#' || text[0] == '\'' || is_parenthesis(text[0]))
        {
            return tokenizer_result_t{ token_t(1, text[0]), drop(text, 1) };
        }
        if (auto v = read_quoted_string(text))
        {
            return *v;
        }

        const auto b = std::begin(text);
        const auto e = std::end(text);

        const auto iter = std::find_if(b, e, [](char ch) { return is_space(ch) || is_parenthesis(ch); });
        if (iter != e)
        {
            return is_space(*iter)  //
                       ? tokenizer_result_t{ token_t{ b, iter }, to_string_view(iter + 1, e) }
                       : tokenizer_result_t{ token_t{ b, iter }, to_string_view(iter, e) };
        }
        else
        {
            return tokenizer_result_t{ token_t{ std::move(text) }, std::string_view{} };
        }
        return std::nullopt;
    }
} tokenize;

constexpr inline struct parse_fn
{
    enum class read_from_mode
    {
        standard,
        tagged
    };

    auto operator()(std::string_view text) const -> value_t
    {
        std::vector<token_t> tokens = tokenize(text);
        std::vector<value_t> values;
        while (!tokens.empty())
        {
            values.push_back(read_from(tokens, read_from_mode::standard));
        }
        if (values.size() != 1)
        {
            throw detail::exception<std::runtime_error>("Exactly one value required");
        }
        return values.at(0);
    }

private:
    template <class T>
    static T pop_front(std::vector<T>& v)
    {
        if (v.empty())
        {
            throw detail::exception<std::runtime_error>("Cannot pop from empty vector");
        }
        T result = v.front();
        v.erase(std::begin(v));
        return result;
    }

    template <class T>
    static auto try_parse(const std::string& txt) -> std::optional<T>
    {
        std::stringstream ss;
        ss << txt;
        T res;
        ss >> res;
        return ss ? std::optional<T>{ std::move(res) } : std::nullopt;
    }

    static auto as_string(const token_t& tok) -> std::optional<value_t>
    {
        if (is_quotation_mark(tok.front()) && is_quotation_mark(tok.back()))
        {
            return string_t{ tok.substr(1, tok.size() - 2).c_str() };
        }
        return std::nullopt;
    }

    static auto as_integer(const token_t& tok) -> std::optional<value_t>
    {
        if (std::find(tok.begin(), tok.end(), '.') == tok.end())
        {
            return try_parse<integer_t>(tok);
        }
        return std::nullopt;
    }

    static auto as_floating_point(const token_t& tok) -> std::optional<value_t>
    {
        return try_parse<floating_point_t>(tok);
    }

    static auto as_boolean(const token_t& tok) -> std::optional<value_t>
    {
        if (tok == "true")
        {
            return boolean_t{ true };
        }
        if (tok == "false")
        {
            return boolean_t{ false };
        }
        return std::nullopt;
    }

    static auto as_nil(const token_t& tok) -> std::optional<value_t>
    {
        return tok == "nil" ? std::optional<value_t>{ nil_t{} } : std::nullopt;
    }

    static auto as_symbol(const token_t& tok) -> std::optional<value_t>
    {
        return symbol_t{ tok.c_str() };
    }

    static auto as_character(const token_t& tok) -> std::optional<value_t>
    {
        if (tok.empty() || tok[0] != '\\')
        {
            return std::nullopt;
        }
        if (tok.size() == 2 && std::isprint(tok[1]))
        {
            return character_t{ tok[1] };
        }
        for (const auto& [symbol, name] : character_names)
        {
            if (tok.substr(1) == name)
            {
                return character_t{ symbol };
            }
        }
        return std::nullopt;
    }

    static auto as_keyword(const token_t& tok) -> std::optional<value_t>
    {
        return tok[0] == ':' ? std::optional<value_t>{ keyword_t{ tok.substr(1).c_str() } } : std::nullopt;
    }

    static auto read_atom(const token_t& tok) -> value_t
    {
        using handler_t = std::optional<value_t> (*)(const token_t&);
        static const std::vector<handler_t> handlers = {
            &parse_fn::as_boolean,         //
            &parse_fn::as_nil,             //
            &parse_fn::as_string,          //
            &parse_fn::as_keyword,         //
            &parse_fn::as_integer,         //
            &parse_fn::as_floating_point,  //
            &parse_fn::as_character,       //
            &parse_fn::as_symbol,          //
        };
        for (const handler_t& handler : handlers)
        {
            if (const auto v = handler(tok))
            {
                return *v;
            }
        }
        throw detail::exception<std::runtime_error>("Unrecognized token `", tok, "`");
    }

    static auto to_map(const std::vector<value_t>& items) -> map_t
    {
        if (items.size() % 2 != 0)
        {
            throw detail::exception<std::runtime_error>("Map expects to have even number of elements");
        }
        auto result = map_t();
        for (std::size_t i = 0; i < items.size(); i += 2)
        {
            result.emplace(items[i + 0], items[i + 1]);
        }
        return result;
    }

    static auto read_until(std::vector<token_t>& tokens, const token_t& delimiter) -> std::vector<value_t>
    {
        auto result = std::vector<value_t>{};
        if (tokens.empty())
        {
            throw detail::exception<std::runtime_error>("invalid parentheses");
        }
        while (!tokens.empty() && tokens.front() != delimiter)
        {
            value_t v = read_from(tokens, read_from_mode::standard);
            result.push_back(std::move(v));
        }
        pop_front(tokens);
        return result;
    }

    static auto read_from(std::vector<token_t>& tokens, read_from_mode mode) -> value_t
    {
        if (tokens.empty())
        {
            return value_t();
        }
        const token_t front = pop_front(tokens);
        if (front == "'")
        {
            value_t arg = read_from(tokens, read_from_mode::standard);
            return list_t{ symbol_t{ "'" }, std::move(arg) };
        }
        if (front == "(")
        {
            const std::vector<value_t> items = read_until(tokens, ")");
            return list_t{ items.begin(), items.end() };
        }
        else if (front == "[")
        {
            const std::vector<value_t> items = read_until(tokens, "]");
            return vector_t{ items.begin(), items.end() };
        }
        else if (front == "#")
        {
            value_t rest = read_from(tokens, read_from_mode::tagged);
            if (const auto v = try_as<vector_t>(rest))
            {
                return set_t(v->begin(), v->end());
            }
            else if (const auto v = try_as<symbol_t>(rest))
            {
                value_t val = read_from(tokens, read_from_mode::standard);
                return tagged_element_t{ *v, std::move(val) };
            }
            else
            {
                throw detail::exception<std::runtime_error>(
                    "# must be followed by {...} (for set) or by symbol (for tagged element)");
            }
        }
        else if (front == "{")
        {
            std::vector<value_t> items = read_until(tokens, "}");
            return mode == read_from_mode::standard  //
                       ? value_t{ to_map(items) }
                       : value_t{ vector_t{ items.begin(), items.end() } };
        }
        return read_atom(front);
    }
} parse;

}  // namespace detail

using detail::parse;
using detail::tokenize;

namespace literals
{

inline auto operator""_kw(const char* str, std::size_t) -> keyword_t
{
    return keyword_t{ str };
}

inline auto operator""_sym(const char* str, std::size_t) -> symbol_t
{
    return symbol_t{ str };
}

inline auto operator""_s(const char* str, std::size_t) -> string_t
{
    return string_t{ str };
}

}  // namespace literals

template <class T, class = void>
struct codec;

template <class T>
struct codec_instance
{
    static const codec<T>& get()
    {
        static const codec<T> m_instance = {};
        return m_instance;
    }
};

template <class T>
value_t encode(const T& in)
{
    return codec_instance<T>::get().encode(in);
}

template <class T>
void encode(value_t& out, const T& in)
{
    return out = encode<T>(in);
}

template <class T>
T decode(const value_t& in)
{
    return codec_instance<T>::get().decode(in);
}

template <class T>
void decode(T& out, const value_t& in)
{
    out = decode<T>(in);
}

}  // namespace edn
