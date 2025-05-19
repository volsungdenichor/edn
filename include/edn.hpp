#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
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
#include <vector>

namespace edn
{

template <class... Args>
auto str(Args&&... args) -> std::string
{
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    return ss.str();
}

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

struct value_t
{
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

    friend std::ostream& operator<<(std::ostream& os, const type_t item)
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

    enum class format_mode_t
    {
        str,
        repr
    };

    struct nil_t
    {
        void format(std::ostream& os, format_mode_t mode) const
        {
            os << "nil";
        }

        friend std::ostream& operator<<(std::ostream& os, const nil_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

    using boolean_t = bool;
    using integer_t = std::int32_t;
    using floating_point_t = double;
    using character_t = char;

    struct string_t : public std::string
    {
        using base_t = std::string;
        using base_t::base_t;

        void format(std::ostream& os, format_mode_t mode) const
        {
            if (mode == format_mode_t::repr)
            {
                os << '"' << (const base_t&)(*this) << '"';
            }
            else
            {
                os << (const base_t&)(*this);
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const string_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
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

        void format(std::ostream& os, format_mode_t mode) const
        {
            (void)mode;
            os << ':' << (const base_t&)(*this);
        }

        friend std::ostream& operator<<(std::ostream& os, const keyword_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

    struct tagged_element_t : public std::string
    {
        using base_t = std::string;
        using base_t::base_t;

        void format(std::ostream& os, format_mode_t mode) const
        {
            (void)mode;
            os << '#' << (const base_t&)(*this);
        }

        friend std::ostream& operator<<(std::ostream& os, const tagged_element_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

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

    struct list_t : public std::vector<value_t>
    {
        using base_t = std::vector<value_t>;
        using base_t::base_t;

        void format(std::ostream& os, format_mode_t mode) const
        {
            os << "(";
            format_range(os, *this, [&](std::ostream& s, const auto& it) { it.format(s, mode); });
            os << ")";
        }

        friend std::ostream& operator<<(std::ostream& os, const list_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

    struct vector_t : public std::vector<value_t>
    {
        using base_t = std::vector<value_t>;
        using base_t::base_t;

        void format(std::ostream& os, format_mode_t mode) const
        {
            os << "[";
            format_range(os, *this, [&](std::ostream& s, const auto& it) { it.format(s, mode); });
            os << "]";
        }

        friend std::ostream& operator<<(std::ostream& os, const vector_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

    struct set_t : public std::set<value_t>
    {
        using base_t = std::set<value_t>;
        using base_t::base_t;

        void format(std::ostream& os, format_mode_t mode) const
        {
            os << "#{";
            format_range(os, *this, [&](std::ostream& s, const auto& it) { it.format(s, mode); });
            os << "}";
        }

        friend std::ostream& operator<<(std::ostream& os, const set_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

    struct map_t : public std::map<value_t, value_t>
    {
        using base_t = std::map<value_t, value_t>;
        using base_t::base_t;

        void format(std::ostream& os, format_mode_t mode) const
        {
            os << "{";
            format_range(
                os,
                *this,
                [&](std::ostream& s, const auto& it)
                {
                    it.first.format(s, mode);
                    s << " ";
                    it.second.format(s, mode);
                });
            os << "}";
        }

        friend std::ostream& operator<<(std::ostream& os, const map_t& item)
        {
            item.format(os, format_mode_t::repr);
            return os;
        }
    };

    static bool to_boolean(const value_t& v)
    {
        if (const auto b = v.if_boolean())
        {
            return *b;
        }
        return false;
    }

    struct callable_t : public std::function<value_t(span<value_t>)>
    {
        using base_t = std::function<value_t(span<value_t>)>;
        using base_t::base_t;

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

    type_t m_type;
    union
    {
        nil_t m_nil;
        boolean_t m_boolean;
        integer_t m_integer;
        floating_point_t m_floating_point;
        string_t m_string;
        character_t m_character;
        symbol_t m_symbol;
        keyword_t m_keyword;
        tagged_element_t m_tagged_element;
        list_t m_list;
        vector_t m_vector;
        set_t m_set;
        map_t m_map;
        callable_t m_callable;
    };

    bool is_nil() const
    {
        return m_type == type_t::nil;
    }

    auto if_boolean() const -> const boolean_t*
    {
        return m_type == type_t::boolean ? &m_boolean : nullptr;
    }

    auto if_integer() const -> const integer_t*
    {
        return m_type == type_t::integer ? &m_integer : nullptr;
    }

    auto if_floating_point() const -> const floating_point_t*
    {
        return m_type == type_t::floating_point ? &m_floating_point : nullptr;
    }

    auto if_string() const -> const string_t*
    {
        return m_type == type_t::string ? &m_string : nullptr;
    }

    auto if_character() const -> const character_t*
    {
        return m_type == type_t::character ? &m_character : nullptr;
    }

    auto if_symbol() const -> const symbol_t*
    {
        return m_type == type_t::symbol ? &m_symbol : nullptr;
    }

    auto if_keyword() const -> const keyword_t*
    {
        return m_type == type_t::keyword ? &m_keyword : nullptr;
    }

    auto if_tagged_element() const -> const tagged_element_t*
    {
        return m_type == type_t::tagged_element ? &m_tagged_element : nullptr;
    }

    auto if_list() const -> const list_t*
    {
        return m_type == type_t::list ? &m_list : nullptr;
    }

    auto if_vector() const -> const vector_t*
    {
        return m_type == type_t::vector ? &m_vector : nullptr;
    }

    auto if_set() const -> const set_t*
    {
        return m_type == type_t::set ? &m_set : nullptr;
    }

    auto if_map() const -> const map_t*
    {
        return m_type == type_t::map ? &m_map : nullptr;
    }

    auto if_callable() const -> const callable_t*
    {
        return m_type == type_t::callable ? &m_callable : nullptr;
    }

    template <class T>
    static void destroy(T& item)
    {
        std::allocator<T>{}.destroy(&item);
    }

    template <class T>
    static void copy(T value_t::*field, value_t& out, const value_t& in)
    {
        std::allocator<T>{}.construct(&(out.*field), in.*field);
    }

    template <class T>
    static void move(T value_t::*field, value_t& out, value_t&& in)
    {
        std::allocator<T>{}.construct(&(out.*field), std::move(in.*field));
    }

    value_t(const value_t& other) : m_type(other.m_type)
    {
        switch (m_type)
        {
            case type_t::nil: copy(&value_t::m_nil, *this, other); break;
            case type_t::boolean: copy(&value_t::m_boolean, *this, other); break;
            case type_t::integer: copy(&value_t::m_integer, *this, other); break;
            case type_t::floating_point: copy(&value_t::m_floating_point, *this, other); break;
            case type_t::string: copy(&value_t::m_string, *this, other); break;
            case type_t::character: copy(&value_t::m_character, *this, other); break;
            case type_t::symbol: copy(&value_t::m_symbol, *this, other); break;
            case type_t::keyword: copy(&value_t::m_keyword, *this, other); break;
            case type_t::tagged_element: copy(&value_t::m_tagged_element, *this, other); break;
            case type_t::list: copy(&value_t::m_list, *this, other); break;
            case type_t::vector: copy(&value_t::m_vector, *this, other); break;
            case type_t::set: copy(&value_t::m_set, *this, other); break;
            case type_t::map: copy(&value_t::m_map, *this, other); break;
            case type_t::callable: copy(&value_t::m_callable, *this, other); break;
            default: break;
        }
    }

    value_t(value_t&& other) noexcept : m_type(other.m_type)
    {
        switch (m_type)
        {
            case type_t::nil: move(&value_t::m_nil, *this, std::move(other)); break;
            case type_t::boolean: move(&value_t::m_boolean, *this, std::move(other)); break;
            case type_t::integer: move(&value_t::m_integer, *this, std::move(other)); break;
            case type_t::floating_point: move(&value_t::m_floating_point, *this, std::move(other)); break;
            case type_t::string: move(&value_t::m_string, *this, std::move(other)); break;
            case type_t::character: move(&value_t::m_character, *this, std::move(other)); break;
            case type_t::symbol: move(&value_t::m_symbol, *this, std::move(other)); break;
            case type_t::keyword: move(&value_t::m_keyword, *this, std::move(other)); break;
            case type_t::tagged_element: move(&value_t::m_tagged_element, *this, std::move(other)); break;
            case type_t::list: move(&value_t::m_list, *this, std::move(other)); break;
            case type_t::vector: move(&value_t::m_vector, *this, std::move(other)); break;
            case type_t::set: move(&value_t::m_set, *this, std::move(other)); break;
            case type_t::map: move(&value_t::m_map, *this, std::move(other)); break;
            case type_t::callable: move(&value_t::m_callable, *this, std::move(other)); break;
            default: break;
        }
    }

    value_t& operator=(const value_t& other)
    {
        this->~value_t();
        new (this) value_t{ other };
        return *this;
    }

    value_t& operator=(value_t&& other)
    {
        this->~value_t();
        new (this) value_t{ std::move(other) };
        return *this;
    }

    value_t(nil_t v) : m_type(type_t::nil), m_nil(v)
    {
    }

    value_t(boolean_t v) : m_type(type_t::boolean), m_boolean(v)
    {
    }

    value_t(integer_t v) : m_type(type_t::integer), m_integer(v)
    {
    }

    value_t(floating_point_t v) : m_type(type_t::floating_point), m_floating_point(v)
    {
    }

    value_t(string_t v) : m_type(type_t::string), m_string(std::move(v))
    {
    }

    value_t(character_t v) : m_type(type_t::character), m_character(v)
    {
    }

    value_t(symbol_t v) : m_type(type_t::symbol), m_symbol(std::move(v))
    {
    }

    value_t(const char* v) : value_t(symbol_t{ v })
    {
    }

    value_t(keyword_t v) : m_type(type_t::keyword), m_keyword(std::move(v))
    {
    }

    value_t(tagged_element_t v) : m_type(type_t::tagged_element), m_tagged_element(std::move(v))
    {
    }

    value_t(list_t v) : m_type(type_t::list), m_list(std::move(v))
    {
    }

    value_t(vector_t v) : m_type(type_t::vector), m_vector(std::move(v))
    {
    }

    value_t(set_t v) : m_type(type_t::set), m_set(std::move(v))
    {
    }

    value_t(map_t v) : m_type(type_t::map), m_map(std::move(v))
    {
    }

    value_t(callable_t v) : m_type(type_t::callable), m_callable(std::move(v))
    {
    }

    value_t() : value_t(nil_t{})
    {
    }

    ~value_t()
    {
        switch (m_type)
        {
            case type_t::nil: destroy(m_nil); break;
            case type_t::boolean: destroy(m_boolean); break;
            case type_t::integer: destroy(m_integer); break;
            case type_t::floating_point: destroy(m_floating_point); break;
            case type_t::string: destroy(m_string); break;
            case type_t::character: destroy(m_character); break;
            case type_t::symbol: destroy(m_symbol); break;
            case type_t::keyword: destroy(m_keyword); break;
            case type_t::tagged_element: destroy(m_tagged_element); break;
            case type_t::list: destroy(m_list); break;
            case type_t::vector: destroy(m_vector); break;
            case type_t::set: destroy(m_set); break;
            case type_t::map: destroy(m_map); break;
            case type_t::callable: destroy(m_callable); break;
            default: break;
        }
    }

    void format(std::ostream& os, format_mode_t mode) const
    {
        switch (m_type)
        {
            case type_t::nil: m_nil.format(os, mode); return;
            case type_t::boolean: os << (m_boolean ? "true" : "false"); return;
            case type_t::integer: os << m_integer; return;
            case type_t::floating_point: os << m_floating_point; return;
            case type_t::string: m_string.format(os, mode); return;
            case type_t::character: os << '\\' << m_character; return;
            case type_t::symbol: os << m_symbol; return;
            case type_t::keyword: m_keyword.format(os, mode); return;
            case type_t::tagged_element: m_tagged_element.format(os, mode); return;
            case type_t::list: m_list.format(os, mode); return;
            case type_t::vector: m_vector.format(os, mode); return;
            case type_t::set: m_set.format(os, mode); return;
            case type_t::map: m_map.format(os, mode); return;
            case type_t::callable: os << m_callable; return;
            default: break;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const value_t& item)
    {
        item.format(os, format_mode_t::repr);
        return os;
    }

    friend bool operator==(const value_t& lhs, const value_t& rhs)
    {
        if (lhs.m_type != rhs.m_type)
        {
            return false;
        }

        switch (lhs.m_type)
        {
            case type_t::nil: return true;
            case type_t::boolean: return lhs.m_boolean == rhs.m_boolean;
            case type_t::integer: return lhs.m_integer == rhs.m_integer;
            case type_t::floating_point:
                return std::abs(lhs.m_floating_point - rhs.m_floating_point)
                       < std::numeric_limits<floating_point_t>::epsilon();
            case type_t::string: return lhs.m_string == rhs.m_string;
            case type_t::character: return lhs.m_character == rhs.m_character;
            case type_t::symbol: return lhs.m_symbol == rhs.m_symbol;
            case type_t::keyword: return lhs.m_keyword == rhs.m_keyword;
            case type_t::tagged_element: return lhs.m_tagged_element == rhs.m_tagged_element;
            case type_t::list: return lhs.m_list == rhs.m_list;
            case type_t::vector: return lhs.m_vector == rhs.m_vector;
            case type_t::set: return lhs.m_set == rhs.m_set;
            case type_t::map: return lhs.m_map == rhs.m_map;
            case type_t::callable: return false;
            default: break;
        }
        return false;
    }

    friend bool operator<(const value_t& lhs, const value_t& rhs)
    {
        if (lhs.m_type == rhs.m_type)
        {
            switch (lhs.m_type)
            {
                case type_t::nil: return false;
                case type_t::boolean: return lhs.m_boolean < rhs.m_boolean;
                case type_t::integer: return lhs.m_integer < rhs.m_integer;
                case type_t::floating_point: return lhs.m_floating_point < rhs.m_floating_point;
                case type_t::string: return lhs.m_string < rhs.m_string;
                case type_t::character: return lhs.m_character < rhs.m_character;
                case type_t::symbol: return lhs.m_symbol < rhs.m_symbol;
                case type_t::keyword: return lhs.m_keyword < rhs.m_keyword;
                case type_t::tagged_element: return lhs.m_tagged_element < rhs.m_tagged_element;
                case type_t::list: return lhs.m_list < rhs.m_list;
                case type_t::vector: return lhs.m_vector < rhs.m_vector;
                case type_t::set: return lhs.m_set < rhs.m_set;
                case type_t::map: return lhs.m_map < rhs.m_map;
                case type_t::callable: return false;
                default: break;
            }
            return true;
        }
        return lhs.m_type < rhs.m_type;
    }

    friend bool operator>(const value_t& lhs, const value_t& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const value_t& lhs, const value_t& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator>=(const value_t& lhs, const value_t& rhs)
    {
        return !(lhs < rhs);
    }
};

using string_view = span<char>;

using token_t = std::string;

struct tokenize_fn
{
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

        if (text.empty())
        {
            return {};
        }
        if (text.size() >= 2 && text.slice({}, 2) == string_view{ "#{" })
        {
            return std::tuple{ std::string(text.slice({}, 2)), text.slice(2, {}) };
        }
        if (is_parenthesis(text[0]))
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
    }

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
            text = std::move(remainder);
        }
        return result;
    }
};

static constexpr inline auto tokenize = tokenize_fn{};

template <class T>
T pop_front(std::vector<T>& v)
{
    if (v.empty())
    {
        throw std::runtime_error{ "Cannot pop from empty vector" };
    }
    T result = v.front();
    v.erase(std::begin(v));
    return result;
}

struct parse_fn
{
    static auto as_string(const token_t& tok) -> std::optional<value_t::string_t>
    {
        if (tok.front() == '\"' && tok.back() == '\"')
        {
            return value_t::string_t{ tok.substr(1, tok.size() - 2).c_str() };
        }
        return {};
    }

    static auto as_integer(const token_t& tok) -> std::optional<value_t::integer_t>
    {
        if (std::all_of(std::begin(tok), std::end(tok), [](char ch) { return std::isdigit(ch); }))
        {
            return value_t::integer_t{ std::atoi(tok.c_str()) };
        }
        return {};
    }

    static auto as_floating_point(const token_t& tok) -> std::optional<value_t::floating_point_t>
    {
        std::stringstream ss;
        ss << tok;
        value_t::floating_point_t res;
        ss >> res;
        if (ss)
        {
            return res;
        }
        return {};
    }

    static auto as_boolean(const token_t& tok) -> std::optional<value_t::boolean_t>
    {
        if (tok == "true")
        {
            return value_t::boolean_t{ true };
        }
        if (tok == "false")
        {
            return value_t::boolean_t{ false };
        }
        return {};
    }

    static auto as_nil(const token_t& tok) -> std::optional<value_t::nil_t>
    {
        if (tok == "nil")
        {
            return value_t::nil_t{};
        }
        return {};
    }

    static auto as_symbol(const token_t& tok) -> std::optional<value_t::symbol_t>
    {
        return value_t::symbol_t{ tok.c_str() };
    }

    static auto as_character(const token_t& tok) -> std::optional<value_t::character_t>
    {
        if (tok.empty() || tok[0] != '\\')
        {
            return {};
        }
        if (tok == "\\space")
        {
            return value_t::character_t{ ' ' };
        }
        if (tok == "\\newline")
        {
            return value_t::character_t{ '\n' };
        }
        if (tok == "\\tab")
        {
            return value_t::character_t{ '\t' };
        }
        if (tok.size() == 2 && std::isprint(tok[1]))
        {
            return value_t::character_t{ tok[1] };
        }
        return {};
    }

    static auto as_keyword(const token_t& tok) -> std::optional<value_t::keyword_t>
    {
        if (tok[0] == ':')
        {
            return value_t::keyword_t{ tok.substr(1).c_str() };
        }
        return {};
    }

    static auto as_tagged(const token_t& tok) -> std::optional<value_t::tagged_element_t>
    {
        if (tok[0] == '#')
        {
            return value_t::tagged_element_t{ tok.substr(1).c_str() };
        }
        return {};
    }

    static auto read_atom(const token_t& tok) -> value_t
    {
        if (const auto v = as_string(tok))
        {
            return *v;
        }
        else if (const auto v = as_keyword(tok))
        {
            return *v;
        }
        else if (const auto v = as_tagged(tok))
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
        else if (const auto v = as_boolean(tok))
        {
            return *v;
        }
        else if (const auto v = as_nil(tok))
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
        throw std::runtime_error{ str("Unrecognized token '", tok, "'") };
    }

    static auto read_from(std::vector<token_t>& tokens) -> value_t
    {
        if (tokens.empty())
        {
            return value_t();
        }
        const auto front = pop_front(tokens);
        if (front == "(")
        {
            auto result = value_t::list_t();
            if (tokens.empty())
            {
                throw std::runtime_error{ "list: invalid parentheses (...)" };
            }
            while (!tokens.empty() && tokens.front() != ")")
            {
                result.push_back(read_from(tokens));
            }
            pop_front(tokens);
            return result;
        }
        else if (front == "[")
        {
            auto result = value_t::vector_t();
            if (tokens.empty())
            {
                throw std::runtime_error{ "vec: invalid parentheses [...]" };
            }
            while (!tokens.empty() && tokens.front() != "]")
            {
                result.push_back(read_from(tokens));
            }
            pop_front(tokens);
            return result;
        }
        else if (front == "#{")
        {
            auto result = value_t::set_t();
            if (tokens.empty())
            {
                throw std::runtime_error{ "set: invalid parentheses #{...}" };
            }
            while (!tokens.empty() && tokens.front() != "}")
            {
                result.insert(read_from(tokens));
            }
            pop_front(tokens);
            return result;
        }
        else if (front == "{")
        {
            auto result = value_t::map_t();
            if (tokens.empty())
            {
                throw std::runtime_error{ "map: invalid parentheses {...}" };
            }
            while (!tokens.empty() && tokens.front() != "}")
            {
                value_t k = read_from(tokens);
                value_t v = read_from(tokens);
                result.emplace(std::move(k), std::move(v));
            }
            pop_front(tokens);
            return result;
        }
        else
        {
            return read_atom(front);
        }
    }

    auto operator()(string_view text) const -> value_t
    {
        auto tokens = tokenize(text);
        return read_from(tokens);
    }
};

static constexpr inline auto parse = parse_fn{};

struct stack_t
{
    using frame_type = std::map<value_t::symbol_t, value_t>;
    frame_type frame;
    stack_t* outer;

    stack_t(frame_type frame, stack_t* outer) : frame{ std::move(frame) }, outer{ outer }
    {
    }

    stack_t(stack_t* outer) : stack_t{ frame_type{}, outer }
    {
    }

    const value_t& insert(const value_t::symbol_t& symbol, const value_t& v)
    {
        frame.emplace(symbol, v);
        return v;
    }

    const value_t& get(const value_t::symbol_t& symbol) const
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

        throw std::runtime_error{ str("Unrecognized symbol '", symbol, "'") };
    }

    const value_t& operator[](const value_t::symbol_t& symbol) const
    {
        return get(symbol);
    }
};

struct evaluate_fn
{
    template <class T>
    static auto deref(T* ptr, const std::string& msg) -> T&
    {
        if (!ptr)
        {
            throw std::runtime_error{ msg };
        }
        return *ptr;
    }

    struct clojure_t
    {
        const evaluate_fn& self;
        value_t parameters;
        std::vector<value_t> body;
        stack_t& stack;

        auto operator()(span<value_t> args) -> value_t
        {
            auto new_stack = stack_t{ &stack };

            const std::vector<value_t::symbol_t> params = std::invoke(
                [&]() -> std::vector<value_t::symbol_t>
                {
                    std::vector<value_t::symbol_t> result;
                    const auto p = deref(parameters.if_vector(), "vector required");
                    for (const value_t& v : p)
                    {
                        result.push_back(deref(v.if_symbol(), "symbol required"));
                    }
                    return result;
                });

            if (params.size() >= 2 && params.at(params.size() - 2) == value_t::symbol_t{ "&" })
            {
                const auto regular_params = span<value_t::symbol_t>{ params, }.slice({}, -2);

                if (args.size() < regular_params.size())
                {
                    throw std::runtime_error{ "too few arguments to a function" };
                }

                value_t::list_t rest;
                for (std::size_t i = 0; i < args.size(); ++i)
                {
                    if (i < regular_params.size())
                    {
                        new_stack.insert(regular_params.at(i), args.at(i));
                    }
                    else
                    {
                        rest.push_back(args.at(i));
                    }
                }
                new_stack.insert(params.back(), rest);
            }
            else
            {
                const auto regular_params = span<value_t::symbol_t>{ params };

                if (args.size() < regular_params.size())
                {
                    throw std::runtime_error{ "too few arguments to a function" };
                }

                for (std::size_t i = 0; i < args.size(); ++i)
                {
                    new_stack.insert(regular_params.at(i), args.at(i));
                }
            }

            return self.eval_block(body, new_stack);
        };
    };

    auto eval_block(span<value_t> input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return eval(item, stack); });
    }

    auto eval_let(span<value_t> input, stack_t& stack) const -> value_t
    {
        const auto& bindings = deref(input.at(0).if_vector(), "vector expected");
        auto new_stack = stack_t{ stack_t::frame_type{}, &stack };
        for (std::size_t i = 0; i < bindings.size(); i += 2)
        {
            new_stack.insert(deref(bindings.at(i + 0).if_symbol(), "symbol expected"), eval(bindings.at(i + 1), new_stack));
        }
        return eval_block(input.slice(1, {}), new_stack);
    }

    auto eval_def(span<value_t> input, stack_t& stack) const -> value_t
    {
        return stack.insert(deref(input.at(0).if_symbol(), "symbol expected"), eval(input.at(1), stack));
    }

    auto eval_callable(span<value_t> input, stack_t& stack) const -> value_t::callable_t
    {
        return value_t::callable_t{ clojure_t{ *this, input.at(0), input.slice(1, {}), stack } };
    }

    auto eval_fn(span<value_t> input, stack_t& stack) const -> value_t
    {
        return eval_callable(input, stack);
    }

    auto eval_defn(span<value_t> input, stack_t& stack) const -> value_t
    {
        return stack.insert(deref(input.at(0).if_symbol(), "symbol expected"), eval_callable(input.slice(1, {}), stack));
    }

    auto eval_boolean(const value_t& value, stack_t& stack) const -> bool
    {
        return deref(eval(value, stack).if_boolean(), "boolean expected");
    }

    auto eval_if(span<value_t> input, stack_t& stack) const -> value_t
    {
        return eval_boolean(input.at(0), stack) ? eval(input.at(1), stack) : eval(input.at(2), stack);
    }

    auto eval_cond(span<value_t> input, stack_t& stack) const -> value_t
    {
        for (std::size_t i = 0; i < input.size(); i += 2)
        {
            if (input.at(i + 0) == value_t::keyword_t{ "else" } || eval_boolean(input.at(i + 0), stack))
            {
                return eval(input.at(i + 1), stack);
            }
        }
        return value_t{};
    }

    auto eval_callable(const value_t& head, span<value_t> tail, stack_t& stack) const -> value_t
    {
        const value_t::callable_t callable = deref(eval(head, stack).if_callable(), "callable expected");
        std::vector<value_t> args;
        args.reserve(tail.size());
        std::transform(
            tail.begin(), tail.end(), std::back_inserter(args), [&](const value_t& item) { return eval(item, stack); });
        return callable(args);
    }

    auto eval_do(span<value_t> input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return eval(item, stack); });
    }

    auto eval_list(const value_t::list_t& input, stack_t& stack) const -> value_t
    {
        if (input.empty())
        {
            return input;
        }
        const value_t& head = input.at(0);
        const auto tail = span<value_t>{ input }.slice(1, {});
        if (head == value_t::symbol_t{ "let" })
        {
            return eval_let(tail, stack);
        }
        if (head == value_t::symbol_t{ "def" })
        {
            return eval_def(tail, stack);
        }
        if (head == value_t::symbol_t{ "fn" })
        {
            return eval_fn(tail, stack);
        }
        if (head == value_t::symbol_t{ "defn" })
        {
            return eval_defn(tail, stack);
        }
        if (head == value_t::symbol_t{ "if" })
        {
            return eval_if(tail, stack);
        }
        if (head == value_t::symbol_t{ "cond" })
        {
            return eval_cond(tail, stack);
        }
        if (head == value_t::symbol_t{ "do" })
        {
            return eval_do(tail, stack);
        }
        return eval_callable(head, tail, stack);
    }

    auto eval_vector(const value_t::vector_t& input, stack_t& stack) const -> value_t::vector_t
    {
        value_t::vector_t output;
        output.reserve(input.size());
        std::transform(
            input.begin(), input.end(), std::back_inserter(output), [&](const value_t& item) { return eval(item, stack); });

        return output;
    }

    auto eval_set(const value_t::set_t& input, stack_t& stack) const -> value_t::set_t
    {
        value_t::set_t output;
        std::transform(
            input.begin(),
            input.end(),
            std::inserter(output, output.end()),
            [&](const value_t& item) { return eval(item, stack); });

        return output;
    }

    auto eval_map(const value_t::map_t& input, stack_t& stack) const -> value_t::map_t
    {
        value_t::map_t output;
        for (const auto& [k, v] : input)
        {
            output.emplace(eval(k, stack), eval(v, stack));
        }
        return output;
    }

    auto do_eval(const value_t& value, stack_t& stack) const -> value_t
    {
        if (const auto symbol = value.if_symbol())
        {
            return stack[*symbol];
        }
        if (const auto list = value.if_list())
        {
            return eval_list(*list, stack);
        }
        if (const auto vector = value.if_vector())
        {
            return eval_vector(*vector, stack);
        }
        if (const auto set = value.if_set())
        {
            return eval_set(*set, stack);
        }
        if (const auto map = value.if_map())
        {
            return eval_map(*map, stack);
        }
        return value;
    }

    auto eval(const value_t& value, stack_t& stack) const -> value_t
    {
        try
        {
            return do_eval(value, stack);
        }
        catch (const std::exception& ex)
        {
            std::throw_with_nested(std::runtime_error{ str("Error on evaluating `", value, "`") });
        }
    }

    auto operator()(const value_t& value, stack_t& stack) const -> value_t
    {
        return eval(value, stack);
    }
};

static constexpr inline auto evaluate = evaluate_fn{};

}  // namespace edn
