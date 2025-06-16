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
struct callable_t;
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
    tagged_element_t,
    list_t,
    vector_t,
    set_t,
    map_t,
    callable_t>;

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
    tagged_element,
    list,
    vector,
    set,
    map,
    callable
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
        CASE(tagged_element);
        CASE(list);
        CASE(vector);
        CASE(set);
        CASE(map);
        CASE(callable);
#undef CASE
        default: break;
    }
    return os;
}

namespace detail
{

static constexpr struct str_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> std::string
    {
        std::stringstream ss;
        (ss << ... << std::forward<Args>(args));
        return ss.str();
    }
} str;

static constexpr struct delimit_fn
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

template <class E = std::runtime_error, class... Args>
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

template <class T>
class span
{
public:
    using iterator = const T*;
    using reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

    iterator m_begin, m_end;

    constexpr span(iterator b, iterator e) : m_begin(b), m_end(e)
    {
    }

    constexpr span(iterator b, difference_type s) : span(b, std::next(b, s))
    {
    }

    template <
        class Container,
        class Ptr = decltype(std::declval<Container>().data()),
        class = std::enable_if_t<std::is_constructible_v<iterator, Ptr>>>
    constexpr span(Container& v) : span(v.data(), v.size())
    {
    }

    template <class Type = T, class = std::enable_if_t<std::is_same_v<Type, char>>>
    constexpr span(const char* txt) : span(txt, std::strlen(txt))
    {
    }

    constexpr span() : span(nullptr, nullptr)
    {
    }

    constexpr span(const span&) = default;
    constexpr span(span&&) noexcept = default;

    void swap(span& other) noexcept
    {
        std::swap(m_begin, other.m_begin);
        std::swap(m_end, other.m_end);
    }

    span& operator=(span other)
    {
        swap(other);
        return *this;
    }

    template <class Container, class = std::enable_if_t<std::is_constructible_v<Container, iterator, iterator>>>
    constexpr operator Container() const
    {
        return Container(begin(), end());
    }

    constexpr iterator begin() const noexcept
    {
        return m_begin;
    }

    constexpr iterator end() const noexcept
    {
        return m_end;
    }

    constexpr bool empty() const
    {
        return begin() == end();
    }

    constexpr difference_type ssize() const
    {
        return std::distance(begin(), end());
    }

    constexpr size_type size() const
    {
        return static_cast<size_type>(ssize());
    }

    reference operator[](difference_type n) const
    {
        return *(begin() + n);
    }

    reference at(difference_type n) const
    {
        return (*this)[n];
    }

    reference front() const
    {
        return at(0);
    }

    reference back() const
    {
        return at(ssize() - 1);
    }

    span slice(std::optional<difference_type> start, std::optional<difference_type> stop) const
    {
        static const auto adjust = [](difference_type index, difference_type size) -> difference_type {  //
            return std::clamp<difference_type>(index >= 0 ? index : index + size, 0, size);
        };
        const difference_type s = ssize();
        const difference_type b = start ? adjust(*start, s) : difference_type{ 0 };
        const difference_type e = stop ? adjust(*stop, s) : s;
        return span{ begin() + b, std::max(difference_type{ 0 }, e - b) };
    }

    template <class Pred>
    span take_while(Pred&& pred) const
    {
        auto e = std::find_if_not(begin(), end(), std::ref(pred));
        return span(begin(), e);
    }

    template <class Pred>
    span drop_while(Pred&& pred) const
    {
        auto b = std::find_if_not(begin(), end(), std::ref(pred));
        return span(b, end());
    }

    friend bool operator==(span lhs, const span rhs)
    {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), std::begin(rhs));
    }

    template <class Type = T, class = std::enable_if_t<std::is_same_v<Type, char>>>
    friend std::ostream& operator<<(std::ostream& os, const span item)
    {
        std::copy(item.begin(), item.end(), std::ostream_iterator<char>(os));
        return os;
    }
};

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

struct tagged_element_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const tagged_element_t& item)
    {
        return os << "#" << (const base_t&)item;
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

struct callable_t : public std::function<value_t(span<value_t>)>
{
    using base_t = std::function<value_t(span<value_t>)>;
    using base_t::base_t;

    static bool to_boolean(const value_t& value)
    {
        return std::visit(overloaded{ [](bool v) -> bool { return v; }, [](const auto&) -> bool { return false; } }, value);
    }

    auto call(span<value_t> args) const -> value_t
    {
        return (*this)(args);
    }

    auto call(const value_t& arg) const -> value_t
    {
        return call(span<value_t>(&arg, 1));
    }

    bool test(span<value_t> args) const
    {
        return to_boolean(call(args));
    }

    bool test(const value_t& arg) const
    {
        return to_boolean(call(arg));
    }

    friend std::ostream& operator<<(std::ostream& os, const callable_t& item)
    {
        return os << "<< callable >>";
    }
};

inline type_t type(const value_t& value)
{
    return std::visit(
        overloaded{
            [](nil_t) { return type_t::nil; },
            [](boolean_t) { return type_t::boolean; },
            [](integer_t) { return type_t::integer; },
            [](floating_point_t) { return type_t::floating_point; },
            [](const string_t&) { return type_t::string; },
            [](const symbol_t&) { return type_t::symbol; },
            [](const keyword_t&) { return type_t::keyword; },
            [](const tagged_element_t&) { return type_t::tagged_element; },
            [](const list_t&) { return type_t::list; },
            [](const vector_t&) { return type_t::vector; },
            [](const set_t&) { return type_t::set; },
            [](const map_t&) { return type_t::map; },
            [](const callable_t&) { return type_t::callable; },
        },
        value);
}

inline std::ostream& operator<<(std::ostream& os, const value_t& item)
{
    std::visit(
        overloaded{
            [&](nil_t v) { os << v; },
            [&](boolean_t v) { os << (v ? "true" : "false"); },
            [&](integer_t v) { os << v; },
            [&](floating_point_t v) { os << v; },
            [&](const string_t& v) { os << v; },
            [&](const symbol_t& v) { os << v; },
            [&](const keyword_t& v) { os << v; },
            [&](const tagged_element_t& v) { os << v; },
            [&](const list_t& v) { os << v; },
            [&](const vector_t& v) { os << v; },
            [&](const set_t& v) { os << v; },
            [&](const map_t& v) { os << v; },
            [&](const callable_t& v) { os << v; },
        },
        item);
    return os;
}

inline bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(
        overloaded{ [](nil_t lt, nil_t rt) { return true; },
                    [](boolean_t lt, boolean_t rt) { return lt == rt; },
                    [](integer_t lt, integer_t rt) { return lt == rt; },
                    [](floating_point_t lt, floating_point_t rt)
                    { return std::abs(lt - rt) < std::numeric_limits<floating_point_t>::epsilon(); },
                    [](const string_t& lt, const string_t& rt) { return lt == rt; },
                    [](const symbol_t& lt, const symbol_t& rt) { return lt == rt; },
                    [](const keyword_t& lt, const keyword_t& rt) { return lt == rt; },
                    [](const tagged_element_t& lt, const tagged_element_t& rt) { return lt == rt; },
                    [](const list_t& lt, const list_t& rt) { return lt == rt; },
                    [](const vector_t& lt, const vector_t& rt) { return lt == rt; },
                    [](const set_t& lt, const set_t& rt) { return lt == rt; },
                    [](const map_t& lt, const map_t& rt) { return lt == rt; },
                    [](const callable_t& lt, const callable_t& rt) { return false; },
                    [](const auto& lt, const auto& rt) { return false; } },
        lhs,
        rhs);
}

inline bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(
        overloaded{ [](nil_t lt, nil_t rt) { return false; },
                    [](boolean_t lt, boolean_t rt) { return lt < rt; },
                    [](integer_t lt, integer_t rt) { return lt < rt; },
                    [](floating_point_t lt, floating_point_t rt) { return lt < rt; },
                    [](const string_t& lt, const string_t& rt) { return lt < rt; },
                    [](const symbol_t& lt, const symbol_t& rt) { return lt < rt; },
                    [](const keyword_t& lt, const keyword_t& rt) { return lt < rt; },
                    [](const tagged_element_t& lt, const tagged_element_t& rt) { return lt < rt; },
                    [](const list_t& lt, const list_t& rt) { return lt < rt; },
                    [](const vector_t& lt, const vector_t& rt) { return lt < rt; },
                    [](const set_t& lt, const set_t& rt) { return lt < rt; },
                    [](const map_t& lt, const map_t& rt) { return lt < rt; },
                    [](const callable_t& lt, const callable_t& rt) { return false; },
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

using string_view = span<char>;

struct stack_t
{
    using frame_type = std::map<symbol_t, value_t>;
    frame_type frame;
    stack_t* outer;

    stack_t(frame_type frame, stack_t* outer) : frame{ std::move(frame) }, outer{ outer }
    {
    }

    stack_t(stack_t* outer) : stack_t{ frame_type{}, outer }
    {
    }

    const value_t& insert(const symbol_t& symbol, const value_t& v)
    {
        frame.emplace(symbol, v);
        return v;
    }

    const value_t& get(const symbol_t& symbol) const
    {
        const auto iter = frame.find(symbol);
        if (iter != frame.end())
        {
            return iter->second;
        }
        if (outer)
        {
            return outer->get(symbol);
        }

        throw detail::exception<>("Unrecognized symbol `", symbol, "`");
    }

    const value_t& operator[](const symbol_t& symbol) const
    {
        return get(symbol);
    }
};

namespace detail
{

using token_t = std::string;

struct tokenize_fn
{
    auto operator()(string_view text) const -> std::vector<token_t>
    {
        std::vector<token_t> result;
        while (true)
        {
            const auto res = read_token(text);
            if (!res)
            {
                break;
            }
            auto [token, remainder] = *res;
            if (!token.empty())
            {
                result.push_back(std::move(token));
            }
            text = remainder;
        }
        return result;
    }

private:
    using tokenizer_result_t = std::tuple<token_t, string_view>;

    static auto read_quoted_string(string_view text) -> tokenizer_result_t
    {
        assert(!text.empty());
        auto it = std::begin(text) + 1;
        token_t result = "\"";
        while (it != std::end(text))
        {
            if (it[0] == '\\' && std::distance(it, std::end(text)) > 1 && it[1] == '"')
            {
                result += '"';
                it += 2;
            }
            else if (it[0] == '"')
            {
                result += *it++;
                break;
            }
            else
            {
                result += *it++;
            }
        }
        return tokenizer_result_t{ result, string_view(it, std::end(text)) };
    }

    static auto read_token(string_view text) -> std::optional<tokenizer_result_t>
    {
        static const auto is_parenthesis
            = [](char ch) { return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}'; };
        static const auto is_quotation_mark = [](char ch) { return ch == '"'; };
        static const auto is_space = [](char ch) { return std::isspace(ch) || ch == ','; };
        static const auto is_comment = [](char ch) { return ch == ';'; };
        static const auto is_new_line = [](char ch) { return ch == '\n'; };

        if (text.empty())
        {
            return {};
        }
        if (is_comment(text[0]))
        {
            return read_token(text.drop_while(std::not_fn(is_new_line)));
        }
        if (text[0] == '#' || text[0] == '\'' || is_parenthesis(text[0]))
        {
            return std::tuple{ std::string(1, text[0]), text.slice(1, {}) };
        }
        if (is_quotation_mark(text[0]))
        {
            return read_quoted_string(text);
        }

        const auto b = std::begin(text);
        const auto e = std::end(text);

        const auto iter = std::find_if(b, e, [](char ch) { return is_space(ch) || is_parenthesis(ch); });
        if (iter != e)
        {
            if (is_space(*iter))
            {
                return tokenizer_result_t{ token_t{ b, iter }, string_view(iter + 1, e) };
            }
            else
            {
                return tokenizer_result_t{ token_t{ b, iter }, string_view(iter, e) };
            }
        }
        else
        {
            return tokenizer_result_t{ token_t{ std::move(text) }, string_view{} };
        }
        return {};
    }
};

static constexpr inline auto tokenize = tokenize_fn{};

struct parse_fn
{
    auto operator()(string_view text) const -> value_t
    {
        std::vector<token_t> tokens = tokenize(text);
        std::vector<value_t> values;
        while (!tokens.empty())
        {
            values.push_back(read_from(tokens));
        }
        if (values.size() == 1)
        {
            return values.at(0);
        }
        list_t result;
        result.push_back(symbol_t{ "do" });
        result.insert(result.end(), values.begin(), values.end());
        return result;
    }

private:
    template <class T>
    static T pop_front(std::vector<T>& v)
    {
        if (v.empty())
        {
            throw detail::exception<>("Cannot pop from empty vector");
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
        return ss ? std::optional<T>{ std::move(res) } : std::optional<T>{};
    }

    static auto as_string(const token_t& tok) -> std::optional<string_t>
    {
        if (tok.front() == '\"' && tok.back() == '\"')
        {
            return string_t{ tok.substr(1, tok.size() - 2).c_str() };
        }
        return {};
    }

    static auto as_integer(const token_t& tok) -> std::optional<integer_t>
    {
        if (std::find(tok.begin(), tok.end(), '.') == tok.end())
        {
            return try_parse<integer_t>(tok);
        }
        return {};
    }

    static auto as_floating_point(const token_t& tok) -> std::optional<floating_point_t>
    {
        return try_parse<floating_point_t>(tok);
    }

    static auto as_boolean(const token_t& tok) -> std::optional<boolean_t>
    {
        if (tok == "true")
        {
            return boolean_t{ true };
        }
        if (tok == "false")
        {
            return boolean_t{ false };
        }
        return {};
    }

    static auto as_nil(const token_t& tok) -> std::optional<nil_t>
    {
        if (tok == "nil")
        {
            return nil_t{};
        }
        return {};
    }

    static auto as_symbol(const token_t& tok) -> std::optional<symbol_t>
    {
        return symbol_t{ tok.c_str() };
    }

    static const inline std::vector<std::tuple<char, std::string>> character_names = {
        { ' ', "space" },
        { '\n', "newline" },
        { '\t', "tab" },
    };

    static auto as_character(const token_t& tok) -> std::optional<character_t>
    {
        if (tok.empty() || tok[0] != '\\')
        {
            return {};
        }
        for (const auto& [symbol, name] : character_names)
        {
            if (tok.substr(1) == name)
            {
                return character_t{ symbol };
            }
        }
        if (tok.size() == 2 && std::isprint(tok[1]))
        {
            return character_t{ tok[1] };
        }
        return {};
    }

    static auto as_keyword(const token_t& tok) -> std::optional<keyword_t>
    {
        if (tok[0] == ':')
        {
            return keyword_t{ tok.substr(1).c_str() };
        }
        return {};
    }

    static auto as_tagged(const token_t& tok) -> std::optional<tagged_element_t>
    {
        if (tok[0] == '#')
        {
            return tagged_element_t{ tok.substr(1).c_str() };
        }
        return {};
    }

    static auto read_atom(const token_t& tok) -> value_t
    {
        if (const auto v = as_boolean(tok))
        {
            return *v;
        }
        else if (const auto v = as_nil(tok))
        {
            return *v;
        }
        if (const auto v = as_string(tok))
        {
            return *v;
        }
        else if (const auto v = as_keyword(tok))
        {
            return *v;
        }
        else if (const auto v = as_integer(tok))
        {
            return *v;
        }
        else if (const auto v = as_floating_point(tok))
        {
            return *v;
        }
        else if (const auto v = as_character(tok))
        {
            return *v;
        }
        else if (const auto v = as_symbol(tok))
        {
            return *v;
        }
        throw detail::exception<>("Unrecognized token `", tok, "`");
    }

    static auto to_map(const std::vector<value_t>& items) -> map_t
    {
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
            throw exception<>("invalid parentheses");
        }
        while (!tokens.empty() && tokens.front() != delimiter)
        {
            value_t v = read_from(tokens);
            result.push_back(std::move(v));
        }
        pop_front(tokens);
        return result;
    }

    static auto read_from(std::vector<token_t>& tokens, bool tagged = false) -> value_t
    {
        if (tokens.empty())
        {
            return value_t();
        }
        const auto front = pop_front(tokens);
        if (front == "'")
        {
            auto arg = read_from(tokens);
            return list_t{ symbol_t{ "quote" }, std::move(arg) };
        }
        if (front == "(")
        {
            const auto items = read_until(tokens, ")");
            return list_t{ items.begin(), items.end() };
        }
        else if (front == "[")
        {
            const auto items = read_until(tokens, "]");
            return vector_t{ items.begin(), items.end() };
        }
        else if (front == "#")
        {
            auto rest = read_from(tokens, true);
            if (const auto v = std::get_if<vector_t>(&rest))
            {
                return set_t(v->begin(), v->end());
            }
            else if (const auto v = std::get_if<symbol_t>(&rest))
            {
                return tagged_element_t{ v->begin(), v->end() };
            }
            else
            {
                throw exception<>("# must be followed by {...} (for set) or by symbol (for tagged element)");
            }
        }
        else if (front == "{")
        {
            auto items = read_until(tokens, "}");
            if (!tagged)
            {
                return to_map(items);
            }
            else
            {
                return vector_t{ items.begin(), items.end() };
            }
        }
        return read_atom(front);
    }
};

static constexpr inline auto parse = parse_fn{};

struct evaluate_fn
{
    auto operator()(const value_t& value, stack_t& stack) const -> value_t
    {
        return eval(value, stack);
    }

private:
    template <class T, class... Args>
    static auto get(const value_t& v, const Args&... args) -> const T&
    {
        const T* ptr = std::get_if<T>(&v);
        if (!ptr)
        {
            throw exception<>(args...);
        }
        return *ptr;
    }

    struct clojure_t
    {
        struct overload_t
        {
            value_t parameters;
            std::vector<value_t> body;

            auto params() const -> std::tuple<std::vector<symbol_t>, std::optional<symbol_t>>
            {
                std::vector<symbol_t> mandatory;
                std::vector<symbol_t> variadic;
                std::vector<symbol_t>* current = &mandatory;
                for (const value_t& v : get<vector_t>(parameters, "vector required"))
                {
                    const symbol_t& s = get<symbol_t>(v, "symbol required");
                    if (s == symbol_t{ "&" })
                    {
                        current = &variadic;
                    }
                    else
                    {
                        current->push_back(s);
                    }
                }
                return { std::move(mandatory),
                         !variadic.empty()  //
                             ? std::optional<symbol_t>{ variadic.at(0) }
                             : std::optional<symbol_t>{} };
            }
        };

        const evaluate_fn& self;
        std::vector<overload_t> overloads;
        stack_t& stack;

        auto operator()(span<value_t> args) -> value_t
        {
            auto new_stack = stack_t{ &stack };

            for (const overload_t& overload : overloads)
            {
                const auto [mandatory, variadic] = overload.params();

                if (args.size() == mandatory.size() && !variadic)
                {
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        new_stack.insert(mandatory.at(i), args.at(i));
                    }
                    return self.eval_block(overload.body, new_stack);
                }
                if (args.size() > mandatory.size() && variadic)
                {
                    list_t tail;
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        if (i < mandatory.size())
                        {
                            new_stack.insert(mandatory.at(i), args.at(i));
                        }
                        else
                        {
                            tail.push_back(args.at(i));
                        }
                    }

                    new_stack.insert(*variadic, tail);
                    return self.eval_block(overload.body, new_stack);
                }
            }
            throw exception<>("could not resolve function overload for ", args.size(), " arg(s)");
        };
    };

    auto eval_block(span<value_t> input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return do_eval(item, stack); });
    }

    auto eval_let(span<value_t> input, stack_t& stack) const -> value_t
    {
        const auto& bindings = get<vector_t>(input.at(0), "vector expected");
        auto new_stack = stack_t{ stack_t::frame_type{}, &stack };
        for (std::size_t i = 0; i < bindings.size(); i += 2)
        {
            new_stack.insert(get<symbol_t>(bindings.at(i + 0), "symbol expected"), do_eval(bindings.at(i + 1), new_stack));
        }
        return eval_block(input.slice(1, {}), new_stack);
    }

    auto eval_def(span<value_t> input, stack_t& stack) const -> value_t
    {
        return stack.insert(get<symbol_t>(input.at(0), "symbol expected"), do_eval(input.at(1), stack));
    }

    static auto create_overload(span<value_t> input) -> clojure_t::overload_t
    {
        value_t parameters = get<vector_t>(input.at(0), "callable: vector required");
        return clojure_t::overload_t{ std::move(parameters), input.slice(1, {}) };
    }

    auto eval_callable(span<value_t> input, stack_t& stack) const -> callable_t
    {
        std::vector<clojure_t::overload_t> overloads;
        if (std::all_of(input.begin(), input.end(), [](const value_t& v) { return std::get_if<list_t>(&v); }))
        {
            for (const value_t& v : input)
            {
                overloads.push_back(create_overload(span<value_t>(get<list_t>(v, "list expected"))));
            }
        }
        else
        {
            overloads.push_back(create_overload(input));
        }
        return callable_t{ clojure_t{ *this, std::move(overloads), stack } };
    }

    auto eval_fn(span<value_t> input, stack_t& stack) const -> value_t
    {
        return eval_callable(input, stack);
    }

    auto eval_defn(span<value_t> input, stack_t& stack) const -> value_t
    {
        return stack.insert(get<symbol_t>(input.at(0), "symbol expected"), eval_callable(input.slice(1, {}), stack));
    }

    auto eval_boolean(const value_t& value, stack_t& stack) const -> bool
    {
        return get<boolean_t>(do_eval(value, stack), "boolean expected");
    }

    auto eval_if(span<value_t> input, stack_t& stack) const -> value_t
    {
        return eval_boolean(input.at(0), stack) ? do_eval(input.at(1), stack) : do_eval(input.at(2), stack);
    }

    auto eval_cond(span<value_t> input, stack_t& stack) const -> value_t
    {
        for (std::size_t i = 0; i < input.size(); i += 2)
        {
            if (input.at(i + 0) == keyword_t{ "else" } || eval_boolean(input.at(i + 0), stack))
            {
                return do_eval(input.at(i + 1), stack);
            }
        }
        return value_t{};
    }

    auto eval_callable(const value_t& head, span<value_t> tail, stack_t& stack) const -> value_t
    {
        const callable_t callable = get<callable_t>(do_eval(head, stack), "callable expected");
        std::vector<value_t> args;
        args.reserve(tail.size());
        for (const value_t& item : tail)
        {
            args.push_back(do_eval(item, stack));
        }
        return callable(args);
    }

    auto eval_do(span<value_t> input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return do_eval(item, stack); });
    }

    auto eval_quote(span<value_t> input, stack_t& stack) const -> value_t
    {
        return input[0];
    }

    auto eval_list(const list_t& input, stack_t& stack) const -> value_t
    {
        if (input.empty())
        {
            return input;
        }
        const value_t& head = input.at(0);
        const auto tail = span<value_t>{ input }.slice(1, {});

        using handler_t = value_t (evaluate_fn::*)(span<value_t>, stack_t&) const;

        static const std::map<symbol_t, handler_t> handlers = {
            { symbol_t{ "quote" }, &evaluate_fn::eval_quote },  //
            { symbol_t{ "let" }, &evaluate_fn::eval_let },      //
            { symbol_t{ "def" }, &evaluate_fn::eval_def },      //
            { symbol_t{ "fn" }, &evaluate_fn::eval_fn },        //
            { symbol_t{ "defn" }, &evaluate_fn::eval_defn },    //
            { symbol_t{ "if" }, &evaluate_fn::eval_if },        //
            { symbol_t{ "cond" }, &evaluate_fn::eval_cond },    //
            { symbol_t{ "do" }, &evaluate_fn::eval_do },        //
        };

        if (const auto h = std::get_if<symbol_t>(&head))
        {
            if (const auto handler = handlers.find(*h); handler != handlers.end())
            {
                return std::invoke(handler->second, this, tail, stack);
            }
        }
        return eval_callable(head, tail, stack);
    }

    auto do_eval(const value_t& value, stack_t& stack) const -> value_t
    {
        return std::visit(
            overloaded{ [&](const symbol_t& v) -> value_t { return stack[v]; },
                        [&](const list_t& v) -> value_t { return eval_list(v, stack); },
                        [&](const vector_t& v) -> value_t
                        {
                            vector_t res;
                            res.reserve(v.size());
                            for (const value_t& item : v)
                            {
                                res.push_back(do_eval(item, stack));
                            }
                            return res;
                        },
                        [&](const set_t& v) -> value_t
                        {
                            set_t res;
                            for (const value_t& item : v)
                            {
                                res.insert(do_eval(item, stack));
                            }
                            return res;
                        },
                        [&](const map_t& v) -> value_t
                        {
                            map_t res;
                            for (const auto& [k, v] : v)
                            {
                                res.emplace(do_eval(k, stack), do_eval(v, stack));
                            }
                            return res;
                        },
                        [](const auto& v) -> value_t { return v; }

            },
            value);
    }

    auto eval(const value_t& value, stack_t& stack) const -> value_t
    {
        try
        {
            return do_eval(value, stack);
        }
        catch (const std::exception& ex)
        {
            throw exception<>("Error on evaluating `", value, "`: ", ex.what());
        }
    }
};

static constexpr inline auto evaluate = evaluate_fn{};

}  // namespace detail

using detail::evaluate;
using detail::parse;
using detail::tokenize;

}  // namespace edn
