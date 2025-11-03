#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace edn
{

constexpr inline struct str_fn
{
    template <class... Args>
    std::string operator()(const Args&... args) const
    {
        std::stringstream ss;
        (ss << ... << args);
        return std::move(ss).str();
    }
} str{};

template <class T>
struct box_t
{
    using value_type = T;
    std::unique_ptr<value_type> m_ptr;

    box_t(value_type value) : m_ptr(std::make_unique<T>(std::move(value)))
    {
    }

    template <class U, class = std::enable_if_t<std::is_constructible_v<value_type, U>>>
    box_t(U&& value) : box_t(value_type{ std::forward<U>(value) })
    {
    }

    box_t(const box_t& other) : box_t(other.get())
    {
    }

    box_t(box_t&&) noexcept = default;

    box_t& operator=(box_t other)
    {
        std::swap(m_ptr, other.m_ptr);
        return *this;
    }

    const value_type& get() const&
    {
        return *m_ptr;
    }

    operator const value_type&() const
    {
        return get();
    }

    friend std::ostream& operator<<(std::ostream& os, const box_t& item)
    {
        return os << item.get();
    }
};

struct value_t;

bool operator==(const value_t& lhs, const value_t& rhs);
bool operator!=(const value_t& lhs, const value_t& rhs);
bool operator<(const value_t& lhs, const value_t& rhs);
bool operator>(const value_t& lhs, const value_t& rhs);
bool operator<=(const value_t& lhs, const value_t& rhs);
bool operator>=(const value_t& lhs, const value_t& rhs);

std::ostream& operator<<(std::ostream& os, const value_t& item);

struct nil_t
{
};

using integer_t = int;
using floating_point_t = double;
using boolean_t = bool;
using character_t = char;

using string_t = std::string;

struct symbol_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const symbol_t& item)
    {
        return os << static_cast<const base_t&>(item);
    }
};

struct keyword_t : public std::string
{
    using base_t = std::string;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const keyword_t& item)
    {
        return os << ":" << static_cast<const base_t&>(item);
    }
};

struct vector_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const vector_t& tem);
};

struct list_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_t& item);
};

struct set_t : public std::set<value_t>
{
    using base_t = std::set<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const set_t& item);
};

struct map_t : public std::map<value_t, value_t>
{
    using base_t = std::map<value_t, value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item);
};

struct tagged_element_t
{
    symbol_t symbol;
    box_t<value_t> element;

    friend std::ostream& operator<<(std::ostream& os, const tagged_element_t& item);
};

struct quoted_element_t
{
    box_t<value_t> element;

    friend std::ostream& operator<<(std::ostream& os, const quoted_element_t& item);
};

struct callable_t : public std::function<value_t(const std::vector<value_t>&)>
{
    using base_t = std::function<value_t(const std::vector<value_t>&)>;
    using base_t::base_t;
};

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

struct print_visitor
{
    std::ostream& os;

    void operator()(nil_t) const
    {
        os << "nil";
    }
    void operator()(integer_t v) const
    {
        os << v;
    }
    void operator()(floating_point_t v) const
    {
        os << v;
    }
    void operator()(boolean_t v) const
    {
        os << (v ? "true" : "false");
    }
    void operator()(character_t v) const
    {
        format_character(os, v);
    }
    void operator()(const string_t& v) const
    {
        os << std::quoted(v);
    }
    void operator()(const symbol_t& v) const
    {
        os << v;
    }
    void operator()(const keyword_t& v) const
    {
        os << v;
    }
    void operator()(const vector_t& v) const
    {
        os << v;
    }
    void operator()(const list_t& v) const
    {
        os << v;
    }
    void operator()(const set_t& v) const
    {
        os << v;
    }
    void operator()(const map_t& v) const
    {
        os << v;
    }
    void operator()(const tagged_element_t& v) const
    {
        os << v;
    }
    void operator()(const quoted_element_t& v) const
    {
        os << v;
    }
    void operator()(const callable_t& v) const
    {
        os << "<< callable >>";
    }
};

enum class value_type_t
{
    nil,
    integer,
    floating_point,
    boolean,
    character,
    string,
    symbol,
    keyword,
    vector,
    list,
    set,
    map,
    tagged_element,
    quoted_element,
    callable
};

inline std::ostream& operator<<(std::ostream& os, const value_type_t item)
{
    switch (item)
    {
        case value_type_t::nil: return os << "nil";
        case value_type_t::integer: return os << "integer";
        case value_type_t::floating_point: return os << "floating_point";
        case value_type_t::boolean: return os << "boolean";
        case value_type_t::character: return os << "character";
        case value_type_t::string: return os << "string";
        case value_type_t::symbol: return os << "symbol";
        case value_type_t::keyword: return os << "keyword";
        case value_type_t::vector: return os << "vector";
        case value_type_t::list: return os << "list";
        case value_type_t::set: return os << "set";
        case value_type_t::map: return os << "map";
        case value_type_t::tagged_element: return os << "tagged_element";
        case value_type_t::quoted_element: return os << "quoted_element";
        case value_type_t::callable: return os << "callable";
    }
    return os;
}

struct get_type_visitor
{
    constexpr auto operator()(nil_t) const -> value_type_t
    {
        return value_type_t::nil;
    }
    constexpr auto operator()(boolean_t) const -> value_type_t
    {
        return value_type_t::boolean;
    }
    constexpr auto operator()(character_t) const -> value_type_t
    {
        return value_type_t::character;
    }
    constexpr auto operator()(integer_t) const -> value_type_t
    {
        return value_type_t::integer;
    }
    constexpr auto operator()(floating_point_t) const -> value_type_t
    {
        return value_type_t::floating_point;
    }
    constexpr auto operator()(const string_t&) const -> value_type_t
    {
        return value_type_t::string;
    }
    constexpr auto operator()(const symbol_t&) const -> value_type_t
    {
        return value_type_t::symbol;
    }
    constexpr auto operator()(const keyword_t&) const -> value_type_t
    {
        return value_type_t::keyword;
    }
    constexpr auto operator()(const list_t&) const -> value_type_t
    {
        return value_type_t::list;
    }
    constexpr auto operator()(const vector_t&) const -> value_type_t
    {
        return value_type_t::vector;
    }
    constexpr auto operator()(const set_t&) const -> value_type_t
    {
        return value_type_t::set;
    }
    constexpr auto operator()(const map_t&) const -> value_type_t
    {
        return value_type_t::map;
    }
    constexpr auto operator()(const tagged_element_t&) const -> value_type_t
    {
        return value_type_t::tagged_element;
    }
    constexpr auto operator()(const quoted_element_t&) const -> value_type_t
    {
        return value_type_t::quoted_element;
    }
    constexpr auto operator()(const callable_t&) const -> value_type_t
    {
        return value_type_t::callable;
    }
};

using data_type = std::variant<  //
    nil_t,
    integer_t,
    floating_point_t,
    boolean_t,
    character_t,
    string_t,
    symbol_t,
    keyword_t,
    box_t<vector_t>,
    box_t<list_t>,
    box_t<set_t>,
    box_t<map_t>,
    tagged_element_t,
    quoted_element_t,
    callable_t>;

template <class T, value_type_t V, bool Boxed = false>
struct access_t
{
    const data_type& m_data;

    constexpr auto type() const -> value_type_t
    {
        return std::visit(get_type_visitor{}, m_data);
    }

    constexpr auto get() const -> const T*
    {
        if constexpr (Boxed)
        {
            if (auto res = std::get_if<box_t<T>>(&m_data))
            {
                return &res->get();
            }
            return nullptr;
        }
        else
        {
            return std::get_if<T>(&m_data);
        }
    }

    constexpr auto has_value() const -> bool
    {
        return get() != nullptr;
    }

    auto value() const -> const T&
    {
        const T* v = get();
        if (!v)
        {
            throw std::runtime_error{ str("Expected type ", V, ", actual ", type()) };
        }
        return *v;
    }

    auto operator*() const -> const T&
    {
        return value();
    }

    explicit operator bool() const
    {
        return has_value();
    }
};

struct value_t
{
    data_type m_data;

    template <class T, class = std::enable_if_t<std::is_constructible_v<data_type, T>>>
    value_t(T&& value) : m_data(std::move(value))
    {
    }

    value_t() : value_t(nil_t{})
    {
    }

    value_t(const value_t&) = default;
    value_t(value_t&&) noexcept = default;

    value_t& operator=(value_t other)
    {
        std::swap(m_data, other.m_data);
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const value_t& item)
    {
        std::visit(print_visitor{ os }, item.m_data);
        return os;
    }

    value_type_t type() const
    {
        return std::visit(get_type_visitor{}, m_data);
    }

    constexpr auto integer() const -> access_t<integer_t, value_type_t::integer>
    {
        return { m_data };
    }

    constexpr auto floating_point() const -> access_t<floating_point_t, value_type_t::floating_point>
    {
        return { m_data };
    }

    constexpr auto boolean() const -> access_t<boolean_t, value_type_t::boolean>
    {
        return { m_data };
    }

    constexpr auto character() const -> access_t<character_t, value_type_t::character>
    {
        return { m_data };
    }

    constexpr auto string() const -> access_t<string_t, value_type_t::string>
    {
        return { m_data };
    }

    constexpr auto symbol() const -> access_t<symbol_t, value_type_t::symbol>
    {
        return { m_data };
    }

    constexpr auto keyword() const -> access_t<keyword_t, value_type_t::keyword>
    {
        return { m_data };
    }

    constexpr auto vector() const -> access_t<vector_t, value_type_t::vector, true>
    {
        return { m_data };
    }

    constexpr auto list() const -> access_t<list_t, value_type_t::list, true>
    {
        return { m_data };
    }

    constexpr auto set() const -> access_t<set_t, value_type_t::set, true>
    {
        return { m_data };
    }

    constexpr auto map() const -> access_t<map_t, value_type_t::map, true>
    {
        return { m_data };
    }

    constexpr auto tagged_element() const -> access_t<tagged_element_t, value_type_t::tagged_element>
    {
        return { m_data };
    }

    constexpr auto quoted_element() const -> access_t<quoted_element_t, value_type_t::quoted_element>
    {
        return { m_data };
    }

    constexpr auto callable() const -> access_t<callable_t, value_type_t::callable>
    {
        return { m_data };
    }
};

inline std::ostream& operator<<(std::ostream& os, const vector_t& item)
{
    os << "[";
    for (auto it = item.begin(); it != item.end(); ++it)
    {
        if (it != item.begin())
        {
            os << " ";
        }
        os << *it;
    }
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const list_t& item)
{
    os << "(";
    for (auto it = item.begin(); it != item.end(); ++it)
    {
        if (it != item.begin())
        {
            os << " ";
        }
        os << *it;
    }
    os << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const set_t& item)
{
    os << "#{";
    for (auto it = item.begin(); it != item.end(); ++it)
    {
        if (it != item.begin())
        {
            os << " ";
        }
        os << *it;
    }
    os << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const tagged_element_t& item)
{
    return os << "#" << item.symbol << " " << item.element;
}

inline std::ostream& operator<<(std::ostream& os, const quoted_element_t& item)
{
    return os << "'" << item.element;
}

inline std::ostream& operator<<(std::ostream& os, const map_t& item)
{
    os << "{";
    for (auto it = item.begin(); it != item.end(); ++it)
    {
        if (it != item.begin())
        {
            os << " ";
        }
        os << it->first << " " << it->second;
    }
    os << "}";
    return os;
}

struct eq_visitor
{
    bool operator()(nil_t lt, nil_t rt) const
    {
        return true;
    }
    bool operator()(boolean_t lt, boolean_t rt) const
    {
        return lt == rt;
    }
    bool operator()(character_t lt, character_t rt) const
    {
        return lt == rt;
    }
    bool operator()(integer_t lt, integer_t rt) const
    {
        return lt == rt;
    }
    bool operator()(floating_point_t lt, floating_point_t rt) const
    {
        return std::abs(lt - rt) < std::numeric_limits<floating_point_t>::epsilon();
    }
    bool operator()(const string_t& lt, const string_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const symbol_t& lt, const symbol_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const keyword_t& lt, const keyword_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const list_t& lt, const list_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const vector_t& lt, const vector_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const set_t& lt, const set_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const map_t& lt, const map_t& rt) const
    {
        return lt == rt;
    }
    bool operator()(const tagged_element_t& lt, const tagged_element_t& rt) const
    {
        return lt.element.get() == rt.element.get();
    }
    bool operator()(const quoted_element_t& lt, const quoted_element_t& rt) const
    {
        return lt.element.get() == rt.element.get();
    }

    template <class L, class R>
    bool operator()(const L& lt, const R& rt) const
    {
        return false;
    }
};

struct lt_visitor
{
    bool operator()(nil_t lt, nil_t rt) const
    {
        return false;
    }
    bool operator()(boolean_t lt, boolean_t rt) const
    {
        return lt < rt;
    }
    bool operator()(character_t lt, character_t rt) const
    {
        return lt < rt;
    }
    bool operator()(integer_t lt, integer_t rt) const
    {
        return lt < rt;
    }
    bool operator()(floating_point_t lt, floating_point_t rt) const
    {
        return lt < rt;
    }
    bool operator()(const string_t& lt, const string_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const symbol_t& lt, const symbol_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const keyword_t& lt, const keyword_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const list_t& lt, const list_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const vector_t& lt, const vector_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const set_t& lt, const set_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const map_t& lt, const map_t& rt) const
    {
        return lt < rt;
    }
    bool operator()(const tagged_element_t& lt, const tagged_element_t& rt) const
    {
        return lt.element.get() < rt.element.get();
    }
    bool operator()(const quoted_element_t& lt, const quoted_element_t& rt) const
    {
        return lt.element.get() < rt.element.get();
    }

    template <class L, class R>
    bool operator()(const L& lt, const R& rt) const
    {
        return false;
    }
};

inline bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(eq_visitor{}, lhs.m_data, rhs.m_data);
}

inline bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(lt_visitor{}, lhs.m_data, rhs.m_data);
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

}  // namespace edn