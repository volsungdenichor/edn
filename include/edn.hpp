#pragma once

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace edn
{

template <class T>
struct box_t
{
    using value_type = T;
    std::unique_ptr<value_type> m_ptr;

    box_t(const value_type& value) : m_ptr(std::make_unique<T>(value)) { }

    box_t(value_type&& value) : m_ptr(std::make_unique<T>(std::move(value))) { }

    box_t(const box_t& other) : box_t(other.get()) { }

    box_t(box_t&&) noexcept = default;

    box_t& operator=(box_t other) noexcept
    {
        std::swap(m_ptr, other.m_ptr);
        return *this;
    }

    constexpr const value_type& get() const& noexcept
    {
        return *m_ptr;
    }

    constexpr operator const value_type&() const noexcept
    {
        return get();
    }

    friend std::ostream& operator<<(std::ostream& os, const box_t& item)
    {
        return os << item.get();
    }
};

struct unbox_fn
{
    template <class T>
    constexpr const T& operator()(const box_t<T>& box) const noexcept
    {
        return box.get();
    }

    template <class T>
    constexpr T& operator()(box_t<T>& box) const noexcept
    {
        return box.get();
    }

    template <class T>
    constexpr const T& operator()(const T& value) const noexcept
    {
        return value;
    }

    template <class T>
    constexpr T& operator()(T& value) const noexcept
    {
        return value;
    }
};

static constexpr inline auto unbox = unbox_fn{};

template <class Visitor>
struct unboxing_visitor
{
    Visitor m_visitor;

    constexpr unboxing_visitor(Visitor visitor) : m_visitor(std::move(visitor)) { }

    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(std::invoke(m_visitor, unbox(std::forward<Args>(args))...))
    {
        return std::invoke(m_visitor, unbox(std::forward<Args>(args))...);
    }
};

template <class Visitor>
unboxing_visitor(Visitor&&) -> unboxing_visitor<std::decay_t<Visitor>>;

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

struct value_t;

struct nil_t
{
};

static constexpr inline auto nil = nil_t{};

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
    symbol_t m_tag;
    box_t<value_t> m_element;

    const symbol_t& tag() const
    {
        return m_tag;
    }

    const value_t& element() const
    {
        return m_element.get();
    }

    friend std::ostream& operator<<(std::ostream& os, const tagged_element_t& item);
};

struct quoted_element_t
{
    box_t<value_t> m_element;

    const value_t& element() const
    {
        return m_element.get();
    }

    friend std::ostream& operator<<(std::ostream& os, const quoted_element_t& item);
};

struct value_t
{
    using data_type = std::variant<
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
        quoted_element_t>;

    data_type m_data;

    value_t() : m_data(nil_t{}) { }
    value_t(nil_t v) : m_data(std::move(v)) { }
    value_t(integer_t v) : m_data(std::move(v)) { }
    value_t(floating_point_t v) : m_data(std::move(v)) { }
    value_t(boolean_t v) : m_data(std::move(v)) { }
    value_t(character_t v) : m_data(std::move(v)) { }
    value_t(string_t v) : m_data(std::move(v)) { }
    value_t(symbol_t v) : m_data(std::move(v)) { }
    value_t(keyword_t v) : m_data(std::move(v)) { }
    value_t(vector_t v) : m_data(std::move(v)) { }
    value_t(list_t v) : m_data(std::move(v)) { }
    value_t(set_t v) : m_data(std::move(v)) { }
    value_t(map_t v) : m_data(std::move(v)) { }
    value_t(tagged_element_t v) : m_data(std::move(v)) { }
    value_t(quoted_element_t v) : m_data(std::move(v)) { }

    value_t(const value_t&) = default;
    value_t(value_t&&) noexcept = default;

    value_t& operator=(value_t other)
    {
        std::swap(m_data, other.m_data);
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const value_t& item);

    friend constexpr bool operator==(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator!=(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator<(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator>(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator<=(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator>=(const value_t& lhs, const value_t& rhs);

    constexpr value_type_t type() const
    {
        struct visitor
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
            // constexpr auto operator()(const callable_t&) const -> value_type_t
            // {
            //     return value_type_t::callable;
            // }
        };
        return std::visit(unboxing_visitor{ visitor{} }, m_data);
    }

    constexpr bool is_nil() const
    {
        return std::holds_alternative<nil_t>(m_data);
    }

    constexpr const integer_t* if_integer() const
    {
        return std::get_if<integer_t>(&m_data);
    }
    constexpr const floating_point_t* if_floating_point() const
    {
        return std::get_if<floating_point_t>(&m_data);
    }
    constexpr const boolean_t* if_boolean() const
    {
        return std::get_if<boolean_t>(&m_data);
    }
    constexpr const character_t* if_character() const
    {
        return std::get_if<character_t>(&m_data);
    }
    constexpr const string_t* if_string() const
    {
        return std::get_if<string_t>(&m_data);
    }
    constexpr const symbol_t* if_symbol() const
    {
        return std::get_if<symbol_t>(&m_data);
    }
    constexpr const keyword_t* if_keyword() const
    {
        return std::get_if<keyword_t>(&m_data);
    }
    constexpr const vector_t* if_vector() const
    {
        if (auto ptr = std::get_if<box_t<vector_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
    }
    constexpr const list_t* if_list() const
    {
        if (auto ptr = std::get_if<box_t<list_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
    }
    constexpr const set_t* if_set() const
    {
        if (auto ptr = std::get_if<box_t<set_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
    }
    constexpr const map_t* if_map() const
    {
        if (auto ptr = std::get_if<box_t<map_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
    }
    constexpr const tagged_element_t* if_tagged_element() const
    {
        return std::get_if<tagged_element_t>(&m_data);
    }
    constexpr const quoted_element_t* if_quoted_element() const
    {
        return std::get_if<quoted_element_t>(&m_data);
    }
};

inline std::ostream& operator<<(std::ostream& os, const nil_t&)
{
    return os << "nil";
}

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

inline std::ostream& operator<<(std::ostream& os, const tagged_element_t& item)
{
    return os << "#" << item.tag() << " " << item.element();
}

inline std::ostream& operator<<(std::ostream& os, const quoted_element_t& item)
{
    return os << "'" << item.element();
}

namespace detail
{

inline const std::vector<std::tuple<char, std::string_view>>& character_names()
{
    static const std::vector<std::tuple<char, std::string_view>> names = {
        { ' ', "space" },
        { '\n', "newline" },
        { '\t', "tab" },
    };
    return names;
}

void format_character(std::ostream& os, character_t v)
{
    for (const auto& [ch, value] : character_names())
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
    // void operator()(const callable_t& v) const
    // {
    //     os << "<< callable >>";
    // }
};

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
        return std::tie(lt.tag(), lt.element()) == std::tie(rt.tag(), rt.element());
    }
    bool operator()(const quoted_element_t& lt, const quoted_element_t& rt) const
    {
        return lt.element() == rt.element();
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
        return std::tie(lt.tag(), lt.element()) < std::tie(rt.tag(), rt.element());
    }
    bool operator()(const quoted_element_t& lt, const quoted_element_t& rt) const
    {
        return lt.element() < rt.element();
    }

    template <class L, class R>
    bool operator()(const L& lt, const R& rt) const
    {
        return false;
    }
};

}  // namespace detail

inline std::ostream& operator<<(std::ostream& os, const value_t& item)
{
    std::visit(unboxing_visitor{ detail::print_visitor{ os } }, item.m_data);
    return os;
}

inline constexpr bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(unboxing_visitor{ detail::eq_visitor{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(unboxing_visitor{ detail::lt_visitor{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator>(const value_t& lhs, const value_t& rhs)
{
    return rhs < lhs;
}

inline constexpr bool operator<=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs > rhs);
}

inline constexpr bool operator>=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs < rhs);
}

namespace ansi
{
constexpr std::string_view reset = "\033[0m";
constexpr std::string_view bold = "\033[1m";
constexpr std::string_view red = "\033[31m";
constexpr std::string_view green = "\033[32m";
constexpr std::string_view yellow = "\033[33m";
constexpr std::string_view blue = "\033[34m";
constexpr std::string_view magenta = "\033[35m";
constexpr std::string_view cyan = "\033[36m";
constexpr std::string_view white = "\033[37m";
}  // namespace ansi

struct color_scheme
{
    std::string_view reset = ansi::reset;

    std::string_view nil = ansi::white;
    std::string_view boolean = ansi::yellow;
    std::string_view number = ansi::cyan;
    std::string_view character = ansi::green;
    std::string_view string = ansi::green;
    std::string_view symbol = ansi::blue;
    std::string_view keyword = ansi::magenta;
    std::string_view tag = ansi::blue;

    std::string_view bracket = ansi::white;
    std::string_view parenthesis = ansi::white;
    std::string_view brace = ansi::white;
};

struct pretty_print_options
{
    int indent_size = 2;
    std::optional<color_scheme> colors = color_scheme{};
    int max_inline_length = 60;
    bool compact_maps = true;
};

namespace detail
{

class pretty_printer
{
    std::ostream& os;
    const pretty_print_options& m_options;
    int current_indent = 0;

    pretty_printer& write_indent()
    {
        os << std::string(current_indent, ' ');
        return *this;
    }

    pretty_printer& write_newline()
    {
        os << "\n";
        return *this;
    }

    std::string_view write_ansi(std::string_view color_scheme::*type) const
    {
        return m_options.colors ? (*m_options.colors).*type : "";
    }

    std::string_view color_for_type(const value_t& item) const
    {
        if (!m_options.colors)
            return "";

        if (item.is_nil())
        {
            return write_ansi(&color_scheme::nil);
        }
        if (item.if_boolean())
        {
            return write_ansi(&color_scheme::boolean);
        }
        if (item.if_integer())
        {
            return write_ansi(&color_scheme::number);
        }
        if (item.if_floating_point())
        {
            return write_ansi(&color_scheme::number);
        }
        if (item.if_character())
        {
            return write_ansi(&color_scheme::character);
        }
        if (item.if_string())
        {
            return write_ansi(&color_scheme::string);
        }
        if (item.if_symbol())
        {
            return write_ansi(&color_scheme::symbol);
        }
        if (item.if_keyword())
        {
            return write_ansi(&color_scheme::keyword);
        }
        return write_ansi(&color_scheme::reset);
    }

    static bool is_simple_value(const value_t& item)
    {
        return !item.if_list() && !item.if_vector() && !item.if_set() && !item.if_map();
    }

    std::size_t estimate_length(const value_t& item) const
    {
        std::ostringstream temp;
        temp << item;
        return temp.str().length();
    }

    pretty_printer& print_value_inline(const value_t& item)
    {
        os << color_for_type(item) << item << write_ansi(&color_scheme::reset);
        return *this;
    }

    template <class T>
    static bool is_compact(const T& item)
    {
        return item.size() <= 3 && std::all_of(item.begin(), item.end(), is_simple_value);
    }

    void print_vector(const vector_t& item, bool inline_mode)
    {
        os << write_ansi(&color_scheme::bracket) << "[" << write_ansi(&color_scheme::reset);

        if (item.empty())
        {
            os << write_ansi(&color_scheme::bracket) << "]" << write_ansi(&color_scheme::reset);
            return;
        }

        const bool should_inline = inline_mode || is_compact(item);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    os << " ";
                }
                print_value_inline(*it);
            }
        }
        else
        {
            current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        os << write_ansi(&color_scheme::bracket) << "]" << write_ansi(&color_scheme::reset);
    }

    void print_list(const list_t& item, bool inline_mode)
    {
        os << write_ansi(&color_scheme::parenthesis) << "(" << write_ansi(&color_scheme::reset);

        if (item.empty())
        {
            os << write_ansi(&color_scheme::parenthesis) << ")" << write_ansi(&color_scheme::reset);
            return;
        }

        const bool should_inline = inline_mode || is_compact(item);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    os << " ";
                }
                print_value_inline(*it);
            }
        }
        else
        {
            current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        os << write_ansi(&color_scheme::parenthesis) << ")" << write_ansi(&color_scheme::reset);
    }

    void print_set(const set_t& item, bool inline_mode)
    {
        os << write_ansi(&color_scheme::brace) << "#{" << write_ansi(&color_scheme::reset);

        if (item.empty())
        {
            os << write_ansi(&color_scheme::brace) << "}" << write_ansi(&color_scheme::reset);
            return;
        }

        const bool should_inline = inline_mode || is_compact(item);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    os << " ";
                }
                print_value_inline(*it);
            }
        }
        else
        {
            current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        os << write_ansi(&color_scheme::brace) << "}" << write_ansi(&color_scheme::reset);
    }

    void print_map(const map_t& item, bool inline_mode)
    {
        os << write_ansi(&color_scheme::brace) << "{" << write_ansi(&color_scheme::reset);

        if (item.empty())
        {
            os << write_ansi(&color_scheme::brace) << "}" << write_ansi(&color_scheme::reset);
            return;
        }

        const bool should_inline
            = m_options.compact_maps && inline_mode && item.size() <= 2
              && std::all_of(
                  item.begin(),
                  item.end(),
                  [this](const auto& p) { return is_simple_value(p.first) && is_simple_value(p.second); });

        if (should_inline)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    os << " ";
                }
                print_value_inline(it->first);
                os << " ";
                print_value_inline(it->second);
            }
        }
        else
        {
            const int indent_increment = m_options.compact_maps ? 2 : m_options.indent_size;
            current_indent += indent_increment;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(it->first, true);
                os << " ";
                print_value(it->second, true);
            }
            current_indent -= indent_increment;
            write_newline().write_indent();
        }

        os << write_ansi(&color_scheme::brace) << "}" << write_ansi(&color_scheme::reset);
    }

    void print_value(const value_t& item, bool inline_mode)
    {
        if (const auto v = item.if_vector())
        {
            print_vector(*v, inline_mode);
            return;
        }
        if (const auto v = item.if_list())
        {
            print_list(*v, inline_mode);
            return;
        }
        if (const auto v = item.if_set())
        {
            print_set(*v, inline_mode);
            return;
        }
        if (const auto v = item.if_map())
        {
            print_map(*v, inline_mode);
            return;
        }
        if (const auto v = item.if_tagged_element())
        {
            os << write_ansi(&color_scheme::tag) << "#" << write_ansi(&color_scheme::reset);
            os << write_ansi(&color_scheme::tag) << v->tag() << write_ansi(&color_scheme::reset) << " ";
            print_value(v->element(), inline_mode);
            return;
        }
        if (const auto v = item.if_quoted_element())
        {
            os << write_ansi(&color_scheme::tag) << "'" << write_ansi(&color_scheme::reset);
            print_value(v->element(), inline_mode);
            return;
        }
        print_value_inline(item);
    }

public:
    pretty_printer(std::ostream& os, const pretty_print_options& options) : os{ os }, m_options{ options } { }

    std::ostream& operator()(const value_t& item)
    {
        print_value(item, false);
        write_newline();
        return os;
    }
};

}  // namespace detail

inline void pretty_print(std::ostream& os, const value_t& item, const pretty_print_options& options = {})
{
    detail::pretty_printer printer(os, options);
    printer(item);
}

inline std::string to_pretty_string(const value_t& item, const pretty_print_options& options = {})
{
    std::ostringstream ss;
    pretty_print(ss, item, options);
    return ss.str();
}
struct location_t
{
    std::size_t line;
    std::size_t column;

    friend std::ostream& operator<<(std::ostream& os, const location_t& loc)
    {
        return os << "line " << (loc.line + 1) << ", column " << (loc.column + 1);
    }
};

class parse_error : public std::runtime_error
{
public:
    location_t location;

    parse_error(const std::string& message, location_t loc) : std::runtime_error(format_message(message, loc)), location(loc)
    {
    }

private:
    static std::string format_message(const std::string& message, location_t loc)
    {
        std::ostringstream ss;
        ss << "Parse error at " << loc << ": " << message;
        return ss.str();
    }
};

namespace detail
{

struct char_t
{
    char value;
    location_t location;
};

class stream_t
{
    std::string_view m_content;
    std::size_t m_pos = 0;
    location_t m_location = { 0, 0 };

public:
    stream_t(std::string_view content) : m_content(content) { }

    bool eof() const
    {
        return m_pos >= m_content.size();
    }

    char_t peek() const
    {
        if (eof())
        {
            throw parse_error("Unexpected end of input", m_location);
        }
        return { m_content[m_pos], m_location };
    }

    char_t get()
    {
        auto result = peek();
        m_pos++;

        if (result.value == '\n')
        {
            m_location.line++;
            m_location.column = 0;
        }
        else
        {
            m_location.column++;
        }

        return result;
    }

    location_t location() const
    {
        return m_location;
    }

    void skip_whitespace_and_comments()
    {
        while (!eof())
        {
            const char ch = m_content[m_pos];

            if (std::isspace(ch) || ch == ',')
            {
                get();
            }
            else if (ch == ';')
            {
                while (!eof() && m_content[m_pos] != '\n')
                {
                    get();
                }
            }
            else
            {
                break;
            }
        }
    }
};

class parser_t
{
    stream_t& m_stream;

    bool is_delimiter(char ch) const
    {
        static const std::string delimiters = "()[]{};,";
        return std::isspace(ch) || delimiters.find(ch) != std::string::npos;
    }

    std::tuple<std::string, location_t> read_token()
    {
        const location_t start_loc = m_stream.location();
        std::string token = {};

        while (!m_stream.eof())
        {
            char ch = m_stream.peek().value;
            if (is_delimiter(ch))
            {
                break;
            }
            token += m_stream.get().value;
        }

        return { std::move(token), start_loc };
    }

    value_t parse_atom(const std::string& token, location_t loc)
    {
        if (token.empty())
        {
            throw parse_error("Empty token", loc);
        }

        static const std::vector<std::tuple<std::string_view, value_t>> special_literals = {
            { "nil", nil },
            { "true", boolean_t{ true } },
            { "false", boolean_t{ false } },
        };

        for (const auto& [literal, value] : special_literals)
        {
            if (token == literal)
            {
                return value;
            }
        }

        if (std::isdigit(token[0]) || (token.size() > 1 && (token[0] == '+' || token[0] == '-') && std::isdigit(token[1])))
        {
            const bool is_float = token.find('.') != std::string::npos;

            if (is_float)
            {
                try
                {
                    return floating_point_t{ std::stod(token) };
                }
                catch (...)
                {
                    throw parse_error("Invalid floating point number: " + token, loc);
                }
            }
            else
            {
                try
                {
                    return integer_t{ std::stoi(token) };
                }
                catch (...)
                {
                    throw parse_error("Invalid integer: " + token, loc);
                }
            }
        }

        return symbol_t{ token.c_str() };
    }

    value_t parse_string()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume opening quote

        std::string result;

        while (!m_stream.eof())
        {
            const char_t ch = m_stream.get();

            if (ch.value == '"')
            {
                return string_t{ result };
            }
            else if (ch.value == '\\')
            {
                if (m_stream.eof())
                {
                    throw parse_error("Unexpected end of string", m_stream.location());
                }

                const char_t escape = m_stream.get();
                switch (escape.value)
                {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    default: throw parse_error(std::string("Invalid escape sequence: \\") + escape.value, escape.location);
                }
            }
            else
            {
                result += ch.value;
            }
        }

        throw parse_error("Unterminated string", start_loc);
    }

    value_t parse_character()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume backslash

        std::string char_name = {};
        while (!m_stream.eof() && !is_delimiter(m_stream.peek().value))
        {
            char_name += m_stream.get().value;
        }

        if (char_name.empty())
        {
            throw parse_error("Empty character literal", start_loc);
        }

        for (const auto& [ch, value] : character_names())
        {
            if (value == char_name)
            {
                return character_t{ ch };
            }
        }

        if (char_name.size() == 1)
        {
            return character_t{ char_name[0] };
        }

        throw parse_error("Unknown character name: " + char_name, start_loc);
    }

    value_t parse_keyword()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume ':'

        std::string name = {};
        while (!m_stream.eof() && !is_delimiter(m_stream.peek().value))
        {
            name += m_stream.get().value;
        }

        if (name.empty())
        {
            throw parse_error("Empty keyword", start_loc);
        }
        return keyword_t{ name.c_str() };
    }

    value_t parse_list()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume '('

        list_t result = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error("Unterminated list", start_loc);
            }

            if (m_stream.peek().value == ')')
            {
                m_stream.get();
                return result;
            }

            result.push_back(parse_value());
        }
    }

    value_t parse_vector()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume '['

        vector_t result = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error("Unterminated vector", start_loc);
            }

            if (m_stream.peek().value == ']')
            {
                m_stream.get();
                return result;
            }

            result.push_back(parse_value());
        }
    }

    value_t parse_map()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume '{'

        std::vector<value_t> items = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error("Unterminated map", start_loc);
            }

            if (m_stream.peek().value == '}')
            {
                m_stream.get();

                if (items.size() % 2 != 0)
                {
                    throw parse_error("Map requires an even number of elements", start_loc);
                }

                map_t result;
                for (std::size_t i = 0; i < items.size(); i += 2)
                {
                    result[items[i + 0]] = items[i + 1];
                }
                return result;
            }

            items.push_back(parse_value());
        }
    }

    value_t parse_set()
    {
        const location_t start_loc = m_stream.location();

        vector_t items = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error("Unterminated set", start_loc);
            }

            if (m_stream.peek().value == '}')
            {
                m_stream.get();
                return set_t{ items.begin(), items.end() };
            }

            items.push_back(parse_value());
        }
    }

    value_t parse_hash()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume '#'

        m_stream.skip_whitespace_and_comments();

        if (m_stream.eof())
        {
            throw parse_error("Unexpected end after #", start_loc);
        }

        const char next = m_stream.peek().value;

        if (next == '{')
        {
            m_stream.get();  // consume '{'
            return parse_set();
        }
        else
        {
            const auto [tag_name, tag_loc] = read_token();

            if (tag_name.empty())
            {
                throw parse_error("Expected tag name after #", start_loc);
            }

            m_stream.skip_whitespace_and_comments();

            value_t element = parse_value();
            return tagged_element_t{ symbol_t{ tag_name.c_str() }, std::move(element) };
        }
    }

    value_t parse_quote()
    {
        m_stream.get();  // consume '\''
        m_stream.skip_whitespace_and_comments();

        value_t element = parse_value();
        return quoted_element_t{ std::move(element) };
    }

public:
    parser_t(stream_t& stream) : m_stream(stream) { }

    value_t parse_value()
    {
        using parse_fn = value_t (parser_t::*)();

        static const std::vector<std::tuple<char, parse_fn>> parsers = {
            { '"', &parser_t::parse_string }, { '\\', &parser_t::parse_character }, { ':', &parser_t::parse_keyword },
            { '(', &parser_t::parse_list },   { '[', &parser_t::parse_vector },     { '{', &parser_t::parse_map },
            { '#', &parser_t::parse_hash },   { '\'', &parser_t::parse_quote },
        };

        m_stream.skip_whitespace_and_comments();

        if (m_stream.eof())
        {
            throw parse_error("Unexpected end of input", m_stream.location());
        }

        const char ch = m_stream.peek().value;

        for (const auto& [identifier, fn] : parsers)
        {
            if (ch == identifier)
            {
                return (this->*fn)();
            }
        }

        if (ch == ')' || ch == ']' || ch == '}')
        {
            throw parse_error(std::string("Unexpected closing delimiter: ") + ch, m_stream.location());
        }

        auto [token, start_loc] = read_token();
        return parse_atom(token, start_loc);
    }
};

struct parse_fn
{
    value_t operator()(std::string_view text) const
    {
        std::vector<value_t> values = read_values(text);
        if (values.empty())
        {
            return nil;
        }
        else if (values.size() == 1)
        {
            return values[0];
        }
        else
        {
            list_t result = {};
            result.push_back(symbol_t{ "do" });
            result.insert(result.end(), values.begin(), values.end());
            return result;
        }
    }

    static std::vector<value_t> read_values(std::string_view text)
    {
        stream_t stream(text);
        parser_t parser(stream);

        std::vector<value_t> values = {};

        while (true)
        {
            stream.skip_whitespace_and_comments();
            if (stream.eof())
            {
                break;
            }
            values.push_back(parser.parse_value());
        }

        return values;
    }
};

}  // namespace detail

constexpr inline auto parse = detail::parse_fn{};

}  // namespace edn
