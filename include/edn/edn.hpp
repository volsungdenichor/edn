#pragma once

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace edn
{

template <class... Args>
inline auto str(Args&&... args) -> std::string
{
    std::ostringstream ss;
    (ss << ... << std::forward<Args>(args));
    return ss.str();
}

template <class K, class V>
struct ordered_map_t
{
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<key_type, mapped_type>;

    std::vector<value_type> m_items;

    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    ordered_map_t() = default;
    ordered_map_t(const ordered_map_t&) = default;
    ordered_map_t(ordered_map_t&&) noexcept = default;

    ordered_map_t(std::initializer_list<value_type> items)
    {
        for (const auto& v : items)
        {
            insert(v);
        }
    }

    constexpr iterator begin() { return m_items.begin(); }
    constexpr iterator end() { return m_items.end(); }
    constexpr const_iterator begin() const { return m_items.begin(); }
    constexpr const_iterator end() const { return m_items.end(); }
    constexpr const_iterator cbegin() const { return begin(); }
    constexpr const_iterator cend() const { return end(); }
    constexpr bool empty() const { return m_items.empty(); }
    constexpr std::size_t size() const { return m_items.size(); }

    const_iterator find(const key_type& key) const
    {
        return std::find_if(begin(), end(), [&](const auto& p) { return p.first == key; });
    }

    iterator find(const key_type& key)
    {
        return std::find_if(begin(), end(), [&](const auto& p) { return p.first == key; });
    }

    mapped_type& operator[](const key_type& key)
    {
        auto it = find(key);
        if (it != end())
        {
            return it->second;
        }
        m_items.emplace_back(key, mapped_type{});
        return m_items.back().second;
    }

    std::pair<iterator, bool> emplace(const key_type& key, const mapped_type& value)
    {
        auto it = find(key);
        if (it != end())
        {
            return std::make_pair(it, false);
        }
        m_items.emplace_back(key, value);
        return std::make_pair(std::prev(end()), true);
    }

    std::pair<iterator, bool> insert(const value_type& p) { return emplace(p.first, p.second); }

    const mapped_type& at(const key_type& key) const
    {
        auto it = find(key);
        if (it == end())
        {
            throw std::out_of_range{ str("key not found in map_t: ", key) };
        }
        return it->second;
    }

    friend constexpr bool operator==(const ordered_map_t& lhs, const ordered_map_t& rhs)
    {
        return lhs.m_items == rhs.m_items;
    }
    friend constexpr bool operator!=(const ordered_map_t& lhs, const ordered_map_t& rhs) { return !(lhs == rhs); }
    friend constexpr bool operator<(const ordered_map_t& lhs, const ordered_map_t& rhs) { return lhs.m_items < rhs.m_items; }
    friend constexpr bool operator>(const ordered_map_t& lhs, const ordered_map_t& rhs) { return rhs < lhs; }
    friend constexpr bool operator<=(const ordered_map_t& lhs, const ordered_map_t& rhs) { return !(rhs < lhs); }
    friend constexpr bool operator>=(const ordered_map_t& lhs, const ordered_map_t& rhs) { return !(lhs < rhs); }
};

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

    constexpr const value_type& get() const& noexcept { return *m_ptr; }

    constexpr operator const value_type&() const noexcept { return get(); }

    friend std::ostream& operator<<(std::ostream& os, const box_t& item) { return os << item.get(); }
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
struct unboxing_visitor_t
{
    Visitor m_visitor;

    constexpr unboxing_visitor_t(Visitor visitor) : m_visitor(std::move(visitor)) { }

    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(std::invoke(m_visitor, unbox(std::forward<Args>(args))...))
    {
        return std::invoke(m_visitor, unbox(std::forward<Args>(args))...);
    }
};

template <class Visitor>
unboxing_visitor_t(Visitor&&) -> unboxing_visitor_t<std::decay_t<Visitor>>;

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

struct map_t : public ordered_map_t<value_t, value_t>
{
    using base_t = ordered_map_t<value_t, value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item);
};

struct tagged_element_t
{
    symbol_t m_tag;
    box_t<value_t> m_element;

    tagged_element_t(symbol_t tag, const value_t& element) : m_tag(std::move(tag)), m_element(element) { }

    const symbol_t& tag() const { return m_tag; }

    const value_t& element() const { return m_element.get(); }

    friend std::ostream& operator<<(std::ostream& os, const tagged_element_t& item);
};

struct quoted_element_t
{
    box_t<value_t> m_element;

    explicit quoted_element_t(const value_t& element) : m_element(element) { }

    const value_t& element() const { return m_element.get(); }

    friend std::ostream& operator<<(std::ostream& os, const quoted_element_t& item);
};

struct callable_t
{
    using function_type = std::function<value_t(const std::vector<value_t>&)>;
    function_type m_function;
    explicit callable_t(function_type function) : m_function(std::move(function)) { }

    value_t operator()(const std::vector<value_t>& args) const;

    friend std::ostream& operator<<(std::ostream& os, const callable_t&) { return os << "<< callable >>"; }
};

using number_t = std::variant<integer_t, floating_point_t>;

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
        quoted_element_t,
        box_t<callable_t>>;

    data_type m_data;

    value_t() : m_data(nil_t{}) { }
    value_t(nil_t v) : m_data(std::move(v)) { }
    value_t(integer_t v) : m_data(std::move(v)) { }
    value_t(floating_point_t v) : m_data(std::move(v)) { }
    value_t(boolean_t v) : m_data(std::move(v)) { }
    value_t(character_t v) : m_data(std::move(v)) { }
    value_t(string_t v) : m_data(std::move(v)) { }
    value_t(const char* v) : m_data(std::string(v)) { }
    value_t(symbol_t v) : m_data(std::move(v)) { }
    value_t(keyword_t v) : m_data(std::move(v)) { }
    value_t(vector_t v) : m_data(std::move(v)) { }
    value_t(list_t v) : m_data(std::move(v)) { }
    value_t(set_t v) : m_data(std::move(v)) { }
    value_t(map_t v) : m_data(std::move(v)) { }
    value_t(tagged_element_t v) : m_data(std::move(v)) { }
    value_t(quoted_element_t v) : m_data(std::move(v)) { }
    value_t(callable_t v) : m_data(std::move(v)) { }

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
            constexpr auto operator()(nil_t) const -> value_type_t { return value_type_t::nil; }
            constexpr auto operator()(boolean_t) const -> value_type_t { return value_type_t::boolean; }
            constexpr auto operator()(character_t) const -> value_type_t { return value_type_t::character; }
            constexpr auto operator()(integer_t) const -> value_type_t { return value_type_t::integer; }
            constexpr auto operator()(floating_point_t) const -> value_type_t { return value_type_t::floating_point; }
            constexpr auto operator()(const string_t&) const -> value_type_t { return value_type_t::string; }
            constexpr auto operator()(const symbol_t&) const -> value_type_t { return value_type_t::symbol; }
            constexpr auto operator()(const keyword_t&) const -> value_type_t { return value_type_t::keyword; }
            constexpr auto operator()(const list_t&) const -> value_type_t { return value_type_t::list; }
            constexpr auto operator()(const vector_t&) const -> value_type_t { return value_type_t::vector; }
            constexpr auto operator()(const set_t&) const -> value_type_t { return value_type_t::set; }
            constexpr auto operator()(const map_t&) const -> value_type_t { return value_type_t::map; }
            constexpr auto operator()(const tagged_element_t&) const -> value_type_t { return value_type_t::tagged_element; }
            constexpr auto operator()(const quoted_element_t&) const -> value_type_t { return value_type_t::quoted_element; }
            constexpr auto operator()(const callable_t&) const -> value_type_t { return value_type_t::callable; }
        };
        return std::visit(unboxing_visitor_t{ visitor{} }, m_data);
    }

    constexpr bool is_nil() const { return std::holds_alternative<nil_t>(m_data); }

    constexpr const integer_t* if_integer() const { return std::get_if<integer_t>(&m_data); }
    constexpr const floating_point_t* if_floating_point() const { return std::get_if<floating_point_t>(&m_data); }
    constexpr const boolean_t* if_boolean() const { return std::get_if<boolean_t>(&m_data); }
    constexpr const character_t* if_character() const { return std::get_if<character_t>(&m_data); }
    constexpr const string_t* if_string() const { return std::get_if<string_t>(&m_data); }
    constexpr const symbol_t* if_symbol() const { return std::get_if<symbol_t>(&m_data); }
    constexpr const keyword_t* if_keyword() const { return std::get_if<keyword_t>(&m_data); }
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
    constexpr const tagged_element_t* if_tagged_element() const { return std::get_if<tagged_element_t>(&m_data); }
    constexpr const quoted_element_t* if_quoted_element() const { return std::get_if<quoted_element_t>(&m_data); }
    constexpr const callable_t* if_callable() const
    {
        if (auto ptr = std::get_if<box_t<callable_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
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

inline value_t callable_t::operator()(const std::vector<value_t>& args) const
{
    return m_function(args);
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

inline void format_character(std::ostream& os, character_t v)
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

struct print_visitor_t
{
    std::ostream& os;

    void operator()(nil_t) const { os << "nil"; }
    void operator()(integer_t v) const { os << v; }
    void operator()(floating_point_t v) const { os << v; }
    void operator()(boolean_t v) const { os << (v ? "true" : "false"); }
    void operator()(character_t v) const { format_character(os, v); }
    void operator()(const string_t& v) const { os << std::quoted(v); }
    void operator()(const symbol_t& v) const { os << v; }
    void operator()(const keyword_t& v) const { os << v; }
    void operator()(const vector_t& v) const { os << v; }
    void operator()(const list_t& v) const { os << v; }
    void operator()(const set_t& v) const { os << v; }
    void operator()(const map_t& v) const { os << v; }
    void operator()(const tagged_element_t& v) const { os << v; }
    void operator()(const quoted_element_t& v) const { os << v; }
    void operator()(const callable_t& v) const { os << v; }
};

struct eq_visitor_t
{
    bool operator()(nil_t, nil_t) const { return true; }
    bool operator()(boolean_t lt, boolean_t rt) const { return lt == rt; }
    bool operator()(character_t lt, character_t rt) const { return lt == rt; }
    bool operator()(integer_t lt, integer_t rt) const { return lt == rt; }
    bool operator()(floating_point_t lt, floating_point_t rt) const
    {
        return std::abs(lt - rt) < std::numeric_limits<floating_point_t>::epsilon();
    }
    bool operator()(const string_t& lt, const string_t& rt) const { return lt == rt; }
    bool operator()(const symbol_t& lt, const symbol_t& rt) const { return lt == rt; }
    bool operator()(const keyword_t& lt, const keyword_t& rt) const { return lt == rt; }
    bool operator()(const list_t& lt, const list_t& rt) const { return lt == rt; }
    bool operator()(const vector_t& lt, const vector_t& rt) const { return lt == rt; }
    bool operator()(const set_t& lt, const set_t& rt) const { return lt == rt; }
    bool operator()(const map_t& lt, const map_t& rt) const { return lt == rt; }
    bool operator()(const tagged_element_t& lt, const tagged_element_t& rt) const
    {
        return std::tie(lt.tag(), lt.element()) == std::tie(rt.tag(), rt.element());
    }
    bool operator()(const quoted_element_t& lt, const quoted_element_t& rt) const { return lt.element() == rt.element(); }

    template <class L, class R>
    bool operator()(const L&, const R&) const
    {
        return false;
    }
};

struct lt_visitor_t
{
    bool operator()(nil_t, nil_t) const { return false; }
    bool operator()(boolean_t lt, boolean_t rt) const { return lt < rt; }
    bool operator()(character_t lt, character_t rt) const { return lt < rt; }
    bool operator()(integer_t lt, integer_t rt) const { return lt < rt; }
    bool operator()(floating_point_t lt, floating_point_t rt) const { return lt < rt; }
    bool operator()(const string_t& lt, const string_t& rt) const { return lt < rt; }
    bool operator()(const symbol_t& lt, const symbol_t& rt) const { return lt < rt; }
    bool operator()(const keyword_t& lt, const keyword_t& rt) const { return lt < rt; }
    bool operator()(const list_t& lt, const list_t& rt) const { return lt < rt; }
    bool operator()(const vector_t& lt, const vector_t& rt) const { return lt < rt; }
    bool operator()(const set_t& lt, const set_t& rt) const { return lt < rt; }
    bool operator()(const map_t& lt, const map_t& rt) const { return lt < rt; }
    bool operator()(const tagged_element_t& lt, const tagged_element_t& rt) const
    {
        return std::tie(lt.tag(), lt.element()) < std::tie(rt.tag(), rt.element());
    }
    bool operator()(const quoted_element_t& lt, const quoted_element_t& rt) const { return lt.element() < rt.element(); }

    template <class L, class R>
    bool operator()(const L&, const R&) const
    {
        return false;
    }
};

}  // namespace detail

inline std::ostream& operator<<(std::ostream& os, const value_t& item)
{
    std::visit(unboxing_visitor_t{ detail::print_visitor_t{ os } }, item.m_data);
    return os;
}

inline constexpr bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(unboxing_visitor_t{ detail::eq_visitor_t{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(unboxing_visitor_t{ detail::lt_visitor_t{} }, lhs.m_data, rhs.m_data);
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

struct color_scheme_t
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

struct pretty_print_options_t
{
    int indent_size = 2;
    std::optional<color_scheme_t> colors = color_scheme_t{};
    std::size_t max_inline_length = 60;
    bool compact_maps = true;
};

namespace detail
{

class pretty_printer_t
{
    std::ostream& m_os;
    const pretty_print_options_t& m_options;
    int m_current_indent = 0;

    pretty_printer_t& write_indent()
    {
        m_os << std::string(static_cast<std::size_t>(m_current_indent), ' ');
        return *this;
    }

    pretty_printer_t& write_newline()
    {
        m_os << "\n";
        return *this;
    }

    std::string_view write_ansi(std::string_view color_scheme_t::* type) const
    {
        return m_options.colors ? (*m_options.colors).*type : "";
    }

    std::string_view color_for_type(const value_t& item) const
    {
        if (!m_options.colors)
        {
            return "";
        }
        if (item.is_nil())
        {
            return write_ansi(&color_scheme_t::nil);
        }
        if (item.if_boolean())
        {
            return write_ansi(&color_scheme_t::boolean);
        }
        if (item.if_integer())
        {
            return write_ansi(&color_scheme_t::number);
        }
        if (item.if_floating_point())
        {
            return write_ansi(&color_scheme_t::number);
        }
        if (item.if_character())
        {
            return write_ansi(&color_scheme_t::character);
        }
        if (item.if_string())
        {
            return write_ansi(&color_scheme_t::string);
        }
        if (item.if_symbol())
        {
            return write_ansi(&color_scheme_t::symbol);
        }
        if (item.if_keyword())
        {
            return write_ansi(&color_scheme_t::keyword);
        }
        return write_ansi(&color_scheme_t::reset);
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

    pretty_printer_t& print_value_inline(const value_t& item)
    {
        m_os << color_for_type(item) << item << write_ansi(&color_scheme_t::reset);
        return *this;
    }

    template <class T>
    static bool is_compact(const T& item, std::size_t max_size = 3)
    {
        return item.size() <= max_size && std::all_of(item.begin(), item.end(), is_simple_value);
    }

    void print(const vector_t& item, bool inline_mode, std::size_t max_compact_size = 3)
    {
        m_os << write_ansi(&color_scheme_t::bracket) << "[" << write_ansi(&color_scheme_t::reset);

        if (item.empty())
        {
            m_os << write_ansi(&color_scheme_t::bracket) << "]" << write_ansi(&color_scheme_t::reset);
            return;
        }

        const bool should_inline = inline_mode || is_compact(item, max_compact_size);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    m_os << " ";
                }
                print_value_inline(*it);
            }
        }
        else
        {
            m_current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            m_current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        m_os << write_ansi(&color_scheme_t::bracket) << "]" << write_ansi(&color_scheme_t::reset);
    }

    void print(const list_t& item, bool inline_mode, std::size_t max_compact_size = 3)
    {
        m_os << write_ansi(&color_scheme_t::parenthesis) << "(" << write_ansi(&color_scheme_t::reset);

        if (item.empty())
        {
            m_os << write_ansi(&color_scheme_t::parenthesis) << ")" << write_ansi(&color_scheme_t::reset);
            return;
        }

        const bool should_inline = inline_mode || is_compact(item, max_compact_size);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    m_os << " ";
                }
                print_value_inline(*it);
            }
        }
        else
        {
            m_current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            m_current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        m_os << write_ansi(&color_scheme_t::parenthesis) << ")" << write_ansi(&color_scheme_t::reset);
    }

    void print(const set_t& item, bool inline_mode, std::size_t max_compact_size = 3)
    {
        m_os << write_ansi(&color_scheme_t::brace) << "#{" << write_ansi(&color_scheme_t::reset);

        if (item.empty())
        {
            m_os << write_ansi(&color_scheme_t::brace) << "}" << write_ansi(&color_scheme_t::reset);
            return;
        }

        const bool should_inline = inline_mode || is_compact(item, max_compact_size);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    m_os << " ";
                }
                print_value_inline(*it);
            }
        }
        else
        {
            m_current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            m_current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        m_os << write_ansi(&color_scheme_t::brace) << "}" << write_ansi(&color_scheme_t::reset);
    }

    void print(const map_t& item, bool inline_mode)
    {
        m_os << write_ansi(&color_scheme_t::brace) << "{" << write_ansi(&color_scheme_t::reset);

        if (item.empty())
        {
            m_os << write_ansi(&color_scheme_t::brace) << "}" << write_ansi(&color_scheme_t::reset);
            return;
        }

        const bool should_inline = m_options.compact_maps && inline_mode && item.size() <= 2
                                   && std::all_of(
                                       item.begin(),
                                       item.end(),
                                       [](const auto& p) { return is_simple_value(p.first) && is_simple_value(p.second); });

        if (should_inline)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    m_os << " ";
                }
                print_value_inline(it->first);
                m_os << " ";
                print_value_inline(it->second);
            }
        }
        else
        {
            const int indent_increment = m_options.compact_maps ? 2 : m_options.indent_size;
            m_current_indent += indent_increment;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(it->first, true);
                m_os << " ";
                print_value(it->second, true);
            }
            m_current_indent -= indent_increment;
            write_newline().write_indent();
        }

        m_os << write_ansi(&color_scheme_t::brace) << "}" << write_ansi(&color_scheme_t::reset);
    }

    void print_value(const value_t& item, bool inline_mode)
    {
        if (const auto maybe_vector = item.if_vector())
        {
            print(*maybe_vector, inline_mode);
        }
        else if (const auto maybe_list = item.if_list())
        {
            print(*maybe_list, inline_mode);
        }
        else if (const auto maybe_set = item.if_set())
        {
            print(*maybe_set, inline_mode);
        }
        else if (const auto maybe_map = item.if_map())
        {
            print(*maybe_map, inline_mode);
        }
        else if (const auto maybe_tagged_element = item.if_tagged_element())
        {
            m_os << write_ansi(&color_scheme_t::tag) << "#" << write_ansi(&color_scheme_t::reset);
            m_os << write_ansi(&color_scheme_t::tag) << maybe_tagged_element->tag() << write_ansi(&color_scheme_t::reset)
                 << " ";
            print_value(maybe_tagged_element->element(), inline_mode);
        }
        else if (const auto maybe_quoted_element = item.if_quoted_element())
        {
            m_os << write_ansi(&color_scheme_t::tag) << "'" << write_ansi(&color_scheme_t::reset);
            print_value(maybe_quoted_element->element(), inline_mode);
        }
        else
        {
            print_value_inline(item);
        }
    }

public:
    pretty_printer_t(std::ostream& os, const pretty_print_options_t& options) : m_os{ os }, m_options{ options } { }

    std::ostream& operator()(const value_t& item)
    {
        print_value(item, false);
        write_newline();
        return m_os;
    }
};

}  // namespace detail

inline void pretty_print(std::ostream& os, const value_t& item, const pretty_print_options_t& options = {})
{
    detail::pretty_printer_t printer(os, options);
    printer(item);
}

inline std::string to_pretty_string(const value_t& item, const pretty_print_options_t& options = {})
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

    bool eof() const { return m_pos >= m_content.size(); }

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

    location_t location() const { return m_location; }

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
        static const std::string delimiters = "()[]{};,\"\\:#'";
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

    template <class T>
    std::pair<T, location_t> parse_collection(char open_delim, char close_delim, const std::string& error_message)
    {
        (void)open_delim;
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume opening delimiter

        T result = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error(error_message, start_loc);
            }

            if (m_stream.peek().value == close_delim)
            {
                m_stream.get();
                return { std::move(result), start_loc };
            }

            result.push_back(parse_value());
        }
    }

    value_t parse_list() { return parse_collection<list_t>('(', ')', "Unterminated list").first; }

    value_t parse_vector() { return parse_collection<vector_t>('[', ']', "Unterminated vector").first; }

    value_t parse_set()
    {
        vector_t items = parse_collection<vector_t>('{', '}', "Unterminated set").first;
        return set_t{ items.begin(), items.end() };
    }

    value_t parse_map()
    {
        auto&& [items, start_loc] = parse_collection<vector_t>('{', '}', "Unterminated map");

        if (items.size() % 2 != 0)
        {
            throw parse_error("Map requires an even number of elements", start_loc);
        }

        map_t result = {};
        for (std::size_t i = 0; i < items.size(); i += 2)
        {
            result[items[i + 0]] = items[i + 1];
        }
        return result;
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
            return parse_set();
        }
        else
        {
            const auto [tag_name, tag_loc] = read_token();

            if (tag_name.empty())
            {
                throw parse_error("Expected tag name after #", start_loc);
            }

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

inline auto operator""_str(const char* str, std::size_t) -> string_t
{
    return string_t{ str };
}

}  // namespace literals

namespace evaluate_detail
{

enum class arithmetic_operator_t
{
    addition,
    subtraction,
    multiplication,
    division
};

struct arithmetic_t
{
    arithmetic_operator_t operation;
    std::string_view name;

    value_t operator()(const std::vector<value_t>& args) const
    {
        const bool has_identity
            = operation == arithmetic_operator_t::addition || operation == arithmetic_operator_t::multiplication;
        if (!has_identity && args.empty())
        {
            throw std::invalid_argument{ str("operator `", name, "` requires at least one argument") };
        }

        number_t result
            = operation == arithmetic_operator_t::multiplication ? number_t{ integer_t{ 1 } } : number_t{ integer_t{ 0 } };
        std::size_t index = 0;
        if (!has_identity)
        {
            result = as_number(args.front(), name);
            index = 1;
            if (args.size() == 1)
            {
                if (operation == arithmetic_operator_t::subtraction)
                {
                    result = apply_arithmetic(integer_t{ 0 }, result, operation, name);
                }
                else
                {
                    result = apply_arithmetic(integer_t{ 1 }, result, operation, name);
                }
            }
        }

        for (; index < args.size(); ++index)
        {
            result = apply_arithmetic(result, as_number(args.at(index), name), operation, name);
        }

        return std::visit([](auto value) -> value_t { return value; }, result);
    }

    static number_t apply_arithmetic(
        const number_t& lhs, const number_t& rhs, arithmetic_operator_t operation, std::string_view name)
    {
        if (const auto left = std::get_if<integer_t>(&lhs))
        {
            if (const auto right = std::get_if<integer_t>(&rhs))
            {
                switch (operation)
                {
                    case arithmetic_operator_t::addition:
                        if (const auto result = checked_add(*left, *right))
                        {
                            return *result;
                        }
                        break;
                    case arithmetic_operator_t::subtraction:
                        if (const auto result = checked_subtract(*left, *right))
                        {
                            return *result;
                        }
                        break;
                    case arithmetic_operator_t::multiplication:
                        if (const auto result = checked_multiply(*left, *right))
                        {
                            return *result;
                        }
                        break;
                    case arithmetic_operator_t::division:
                        if (*right == 0)
                        {
                            throw std::domain_error{ str("division by zero in `", name, "`") };
                        }
                        if (!(*left == std::numeric_limits<integer_t>::min() && *right == -1) && *left % *right == 0)
                        {
                            return *left / *right;
                        }
                        break;
                }
            }
        }

        const floating_point_t left = as_floating_point(lhs);
        const floating_point_t right = as_floating_point(rhs);
        switch (operation)
        {
            case arithmetic_operator_t::addition: return left + right;
            case arithmetic_operator_t::subtraction: return left - right;
            case arithmetic_operator_t::multiplication: return left * right;
            case arithmetic_operator_t::division:
                if (right == 0.0)
                {
                    throw std::domain_error{ str("division by zero in `", name, "`") };
                }
                return left / right;
        }
        throw std::logic_error{ "unknown arithmetic operator" };
    }

    static number_t as_number(const value_t& value, std::string_view name)
    {
        if (const auto integer = value.if_integer())
        {
            return *integer;
        }
        if (const auto floating_point = value.if_floating_point())
        {
            return *floating_point;
        }
        throw std::invalid_argument{ str("operator `", name, "` requires numeric arguments") };
    }

    static floating_point_t as_floating_point(const number_t& value)
    {
        if (const auto integer = std::get_if<integer_t>(&value))
        {
            return static_cast<floating_point_t>(*integer);
        }
        return std::get<floating_point_t>(value);
    }

    static std::optional<integer_t> checked_add(integer_t lhs, integer_t rhs)
    {
        constexpr auto min = std::numeric_limits<integer_t>::min();
        constexpr auto max = std::numeric_limits<integer_t>::max();
        if ((rhs > 0 && lhs > max - rhs) || (rhs < 0 && lhs < min - rhs))
        {
            return {};
        }
        return lhs + rhs;
    }

    static std::optional<integer_t> checked_subtract(integer_t lhs, integer_t rhs)
    {
        constexpr auto min = std::numeric_limits<integer_t>::min();
        constexpr auto max = std::numeric_limits<integer_t>::max();
        if ((rhs < 0 && lhs > max + rhs) || (rhs > 0 && lhs < min + rhs))
        {
            return {};
        }
        return lhs - rhs;
    }

    static std::optional<integer_t> checked_multiply(integer_t lhs, integer_t rhs)
    {
        constexpr auto min = std::numeric_limits<integer_t>::min();
        constexpr auto max = std::numeric_limits<integer_t>::max();
        if (lhs == 0 || rhs == 0)
        {
            return 0;
        }
        if ((lhs == -1 && rhs == min) || (rhs == -1 && lhs == min))
        {
            return {};
        }
        if ((lhs > 0 && rhs > 0 && lhs > max / rhs) || (lhs > 0 && rhs < 0 && rhs < min / lhs)
            || (lhs < 0 && rhs > 0 && lhs < min / rhs) || (lhs < 0 && rhs < 0 && lhs < max / rhs))
        {
            return {};
        }
        return lhs * rhs;
    }
};

inline bool is_truthy(const value_t& value)
{
    const auto boolean = value.if_boolean();
    return value.type() != value_type_t::nil && (!boolean || *boolean);
}

enum class comparison_operator_t
{
    equal,
    not_equal,
    less,
    less_equal,
    greater,
    greater_equal
};

struct comparison_t
{
    comparison_operator_t operation;
    std::string_view name;

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.empty())
        {
            throw std::invalid_argument{ str("operator `", name, "` requires at least one argument") };
        }
        if (operation == comparison_operator_t::equal || operation == comparison_operator_t::not_equal)
        {
            const bool equal
                = std::all_of(args.begin() + 1, args.end(), [&](const value_t& value) { return args.front() == value; });
            return operation == comparison_operator_t::equal ? equal : !equal;
        }

        auto previous = arithmetic_t::as_floating_point(arithmetic_t::as_number(args.front(), name));
        bool result = true;
        for (auto it = args.begin() + 1; it != args.end(); ++it)
        {
            const auto current = arithmetic_t::as_floating_point(arithmetic_t::as_number(*it, name));
            switch (operation)
            {
                case comparison_operator_t::less: result = (previous < current) && result; break;
                case comparison_operator_t::less_equal: result = (previous <= current) && result; break;
                case comparison_operator_t::greater: result = (previous > current) && result; break;
                case comparison_operator_t::greater_equal: result = (previous >= current) && result; break;
                default: throw std::logic_error{ "unknown numeric comparison operator" };
            }
            previous = current;
        }
        return result;
    }
};

struct predicate_t
{
    using signature_t = bool (*)(const value_t&);
    std::string_view name;
    signature_t test;

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.size() != 1)
        {
            throw std::invalid_argument{ str("`", name, "` requires exactly one argument") };
        }
        return test(args.front());
    }
};

static const callable_t& as_callable(const value_t& value, std::string_view name)
{
    if (const auto callable = value.if_callable())
    {
        return *callable;
    }
    throw std::invalid_argument{ str("`", name, "` requires callable arguments, got ", value) };
}

struct collection_constructor_t
{
    value_type_t type;

    value_t operator()(const std::vector<value_t>& args) const
    {
        switch (type)
        {
            case value_type_t::vector: return vector_t{ args.begin(), args.end() };
            case value_type_t::list: return list_t{ args.begin(), args.end() };
            case value_type_t::set: return set_t{ args.begin(), args.end() };
            case value_type_t::map:
            {
                if (args.size() % 2 != 0)
                {
                    throw std::invalid_argument{ "`hash-map` requires an even number of arguments" };
                }
                map_t result;
                for (std::size_t i = 0; i < args.size(); i += 2)
                {
                    result[args.at(i)] = args.at(i + 1);
                }
                return result;
            }
            default: throw std::logic_error{ "unknown collection constructor" };
        }
    }
};

inline const std::vector<value_t>& as_sequence(const value_t& value, std::string_view name)
{
    if (const auto vector = value.if_vector())
    {
        return *vector;
    }
    if (const auto list = value.if_list())
    {
        return *list;
    }
    if (value.is_nil())
    {
        static const std::vector<value_t> empty;
        return empty;
    }
    throw std::invalid_argument{ str("`", name, "` requires a list, vector, or nil") };
}

struct mapv_t
{
    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.size() < 2)
        {
            throw std::invalid_argument{ "`mapv` requires a function and at least one collection" };
        }
        const auto& function = as_callable(args.front(), "mapv");
        std::vector<const std::vector<value_t>*> inputs;
        std::size_t size = std::numeric_limits<std::size_t>::max();
        for (auto it = args.begin() + 1; it != args.end(); ++it)
        {
            const auto& input = as_sequence(*it, "mapv");
            inputs.push_back(&input);
            size = std::min(size, input.size());
        }
        vector_t result;
        result.reserve(size);
        std::vector<value_t> parameters;
        parameters.reserve(inputs.size());
        for (std::size_t i = 0; i < size; ++i)
        {
            parameters.clear();
            for (const auto input : inputs)
            {
                parameters.push_back(input->at(i));
            }
            result.push_back(function(parameters));
        }
        return result;
    }
};

struct filterv_t
{
    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.size() != 2)
        {
            throw std::invalid_argument{ "`filterv` requires a predicate and one collection" };
        }
        const auto& predicate = as_callable(args.front(), "filterv");
        const auto& input = as_sequence(args.at(1), "filterv");
        vector_t result;
        for (const auto& value : input)
        {
            if (is_truthy(predicate({ value })))
            {
                result.push_back(value);
            }
        }
        return result;
    }
};

struct reduce_t
{
    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.size() != 2 && args.size() != 3)
        {
            throw std::invalid_argument{ "`reduce` requires a function, an optional initial value, and one collection" };
        }
        const auto& function = as_callable(args.front(), "reduce");
        const auto& input = as_sequence(args.back(), "reduce");
        const bool has_initial = args.size() == 3;
        if (!has_initial && input.empty())
        {
            return function({});
        }
        value_t result = has_initial ? args.at(1) : input.front();
        for (std::size_t i = has_initial ? 0 : 1; i < input.size(); ++i)
        {
            result = function({ result, input.at(i) });
        }
        return result;
    }
};

struct partial_t
{
    struct impl_t
    {
        value_t function;
        std::vector<value_t> bound_args;

        value_t operator()(const std::vector<value_t>& args) const
        {
            std::vector<value_t> all_args;
            all_args.reserve(bound_args.size() + args.size());
            all_args.insert(all_args.end(), bound_args.begin(), bound_args.end());
            all_args.insert(all_args.end(), args.begin(), args.end());
            return as_callable(function, "partial")(all_args);
        }
    };

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.empty())
        {
            throw std::invalid_argument{ "`partial` requires at least one argument" };
        }
        as_callable(args.front(), "partial");
        if (args.size() == 1)
        {
            return args.front();
        }
        return callable_t{ impl_t{ args.front(), std::vector<value_t>(args.begin() + 1, args.end()) } };
    }
};

struct comp_t
{
    struct impl_t
    {
        std::vector<value_t> functions;

        value_t operator()(const std::vector<value_t>& args) const
        {
            auto it = functions.rbegin();
            value_t result = as_callable(*it, "comp")(args);
            for (++it; it != functions.rend(); ++it)
            {
                result = as_callable(*it, "comp")(std::vector<value_t>{ result });
            }
            return result;
        }
    };

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.empty())
        {
            return callable_t{ [](const std::vector<value_t>& inner) -> value_t
                               {
                                   if (inner.size() != 1)
                                   {
                                       throw std::invalid_argument{
                                           "identity function returned by `(comp)` requires exactly one argument"
                                       };
                                   }
                                   return inner.front();
                               } };
        }
        for (const value_t& arg : args)
        {
            as_callable(arg, "comp");
        }
        if (args.size() == 1)
        {
            return args.front();
        }
        return callable_t{ impl_t{ args } };
    }
};

}  // namespace evaluate_detail

namespace evaluate_detail
{

using frame_type = std::map<symbol_t, value_t>;

struct scope_t
{
    std::shared_ptr<const scope_t> outer;
    std::weak_ptr<scope_t> root;
    frame_type frame;
    bool is_root = false;
};

}  // namespace evaluate_detail

class stack_t
{
public:
    using frame_type = evaluate_detail::frame_type;

    stack_t() : stack_t{ frame_type{}, nullptr } { }

    stack_t(frame_type frame, const stack_t* outer) : m_scope{ std::make_shared<evaluate_detail::scope_t>() }
    {
        m_scope->frame = std::move(frame);
        m_scope->is_root = outer == nullptr;
        if (outer)
        {
            m_scope->outer = outer->locals();
            m_scope->root = outer->root();
        }
    }

    stack_t(const stack_t* outer) : stack_t{ frame_type{}, outer } { }

    const value_t& insert(const symbol_t& symbol, const value_t& v)
    {
        m_scope->frame.emplace(symbol, v);
        return v;
    }

    const value_t& define(const symbol_t& symbol, const value_t& v)
    {
        const auto root = lock_root();
        root->frame.emplace(symbol, v);
        return v;
    }

    const value_t& get(const symbol_t& symbol) const
    {
        for (const evaluate_detail::scope_t* scope = locals().get(); scope; scope = scope->outer.get())
        {
            if (const auto iter = scope->frame.find(symbol); iter != scope->frame.end())
            {
                return iter->second;
            }
        }

        const auto root = lock_root();
        if (const auto iter = root->frame.find(symbol); iter != root->frame.end())
        {
            return iter->second;
        }
        throw std::runtime_error{ str("Unrecognized symbol `", symbol, "`") };
    }

    const value_t& operator[](const symbol_t& symbol) const { return get(symbol); }

private:
    friend struct evaluate_fn;

    struct captured_t
    {
        std::shared_ptr<const evaluate_detail::scope_t> locals;
        std::weak_ptr<evaluate_detail::scope_t> root;
    };

    stack_t(frame_type frame, const captured_t& captured) : m_scope{ std::make_shared<evaluate_detail::scope_t>() }
    {
        m_scope->frame = std::move(frame);
        m_scope->outer = captured.locals;
        m_scope->root = captured.root;
    }

    auto locals() const -> std::shared_ptr<const evaluate_detail::scope_t> { return m_scope->is_root ? nullptr : m_scope; }

    auto root() const -> std::weak_ptr<evaluate_detail::scope_t> { return m_scope->is_root ? m_scope : m_scope->root; }

    auto capture() const -> captured_t { return { locals(), root() }; }

    auto lock_root() const -> std::shared_ptr<evaluate_detail::scope_t>
    {
        if (auto root_scope = root().lock())
        {
            return root_scope;
        }
        throw std::logic_error{ "the root environment of this scope has been destroyed" };
    }

    std::shared_ptr<evaluate_detail::scope_t> m_scope;
};

inline auto make_standard_environment() -> stack_t
{
    stack_t environment;
    environment.insert(
        symbol_t{ "+" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::addition, "+" } });
    environment.insert(
        symbol_t{ "-" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::subtraction, "-" } });
    environment.insert(
        symbol_t{ "*" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::multiplication, "*" } });
    environment.insert(
        symbol_t{ "/" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::division, "/" } });
    environment.insert(symbol_t{ "partial" }, callable_t{ evaluate_detail::partial_t{} });
    environment.insert(symbol_t{ "comp" }, callable_t{ evaluate_detail::comp_t{} });
    for (const auto& [name, type] : { std::pair{ "vector", value_type_t::vector },
                                      std::pair{ "list", value_type_t::list },
                                      std::pair{ "hash-map", value_type_t::map },
                                      std::pair{ "hash-set", value_type_t::set } })
    {
        environment.insert(symbol_t{ name }, callable_t{ evaluate_detail::collection_constructor_t{ type } });
    }
    environment.insert(symbol_t{ "mapv" }, callable_t{ evaluate_detail::mapv_t{} });
    environment.insert(symbol_t{ "filterv" }, callable_t{ evaluate_detail::filterv_t{} });
    environment.insert(symbol_t{ "reduce" }, callable_t{ evaluate_detail::reduce_t{} });
    using comparison = evaluate_detail::comparison_operator_t;
    for (const auto& [name, operation] : { std::pair{ "=", comparison::equal },
                                           std::pair{ "not=", comparison::not_equal },
                                           std::pair{ "<", comparison::less },
                                           std::pair{ "<=", comparison::less_equal },
                                           std::pair{ ">", comparison::greater },
                                           std::pair{ ">=", comparison::greater_equal } })
    {
        environment.insert(symbol_t{ name }, callable_t{ evaluate_detail::comparison_t{ operation, name } });
    }
    for (const auto& predicate :
         { evaluate_detail::predicate_t{ "nil?", [](const value_t& v) { return v.type() == value_type_t::nil; } },
           evaluate_detail::predicate_t{ "boolean?", [](const value_t& v) { return v.if_boolean() != nullptr; } },
           evaluate_detail::predicate_t{ "true?", [](const value_t& v) { return v.if_boolean() && *v.if_boolean(); } },
           evaluate_detail::predicate_t{ "false?", [](const value_t& v) { return v.if_boolean() && !*v.if_boolean(); } },
           evaluate_detail::predicate_t{ "number?",
                                         [](const value_t& v) { return v.if_integer() || v.if_floating_point(); } },
           evaluate_detail::predicate_t{ "integer?", [](const value_t& v) { return v.if_integer() != nullptr; } },
           evaluate_detail::predicate_t{ "float?", [](const value_t& v) { return v.if_floating_point() != nullptr; } },
           evaluate_detail::predicate_t{ "char?", [](const value_t& v) { return v.if_character() != nullptr; } },
           evaluate_detail::predicate_t{ "string?", [](const value_t& v) { return v.if_string() != nullptr; } },
           evaluate_detail::predicate_t{ "symbol?", [](const value_t& v) { return v.if_symbol() != nullptr; } },
           evaluate_detail::predicate_t{ "keyword?", [](const value_t& v) { return v.if_keyword() != nullptr; } },
           evaluate_detail::predicate_t{ "list?", [](const value_t& v) { return v.if_list() != nullptr; } },
           evaluate_detail::predicate_t{ "vector?", [](const value_t& v) { return v.if_vector() != nullptr; } },
           evaluate_detail::predicate_t{ "set?", [](const value_t& v) { return v.if_set() != nullptr; } },
           evaluate_detail::predicate_t{ "map?", [](const value_t& v) { return v.if_map() != nullptr; } },
           evaluate_detail::predicate_t{ "fn?", [](const value_t& v) { return v.if_callable() != nullptr; } },
           evaluate_detail::predicate_t{
               "coll?", [](const value_t& v) { return v.if_list() || v.if_vector() || v.if_set() || v.if_map(); } },
           evaluate_detail::predicate_t{ "sequential?", [](const value_t& v) { return v.if_list() || v.if_vector(); } },
           evaluate_detail::predicate_t{ "not", [](const value_t& v) { return !evaluate_detail::is_truthy(v); } } })
    {
        environment.insert(symbol_t{ predicate.name }, callable_t{ predicate });
    }
    return environment;
}

constexpr inline struct evaluate_fn
{
    auto operator()(const value_t& value, stack_t& stack) const -> value_t { return eval(value, stack); }

private:
    auto eval(const value_t& value, stack_t& stack) const -> value_t
    {
        try
        {
            return do_eval(value, stack);
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error{ str("Error on evaluating `", value, "`: ", ex.what()) };
        }
    }

    auto do_eval(const value_t& value, stack_t& stack) const -> value_t
    {
        if (auto maybe_quoted_element = value.if_quoted_element())
        {
            return maybe_quoted_element->element();
        }
        else if (auto maybe_symbol = value.if_symbol())
        {
            return stack[*maybe_symbol];
        }
        else if (auto maybe_list = value.if_list())
        {
            return eval_list(*maybe_list, stack);
        }
        else if (auto maybe_vector = value.if_vector())
        {
            vector_t res = {};
            res.reserve(maybe_vector->size());
            for (const value_t& item : *maybe_vector)
            {
                res.push_back(do_eval(item, stack));
            }
            return res;
        }
        else if (auto maybe_set = value.if_set())
        {
            set_t res = {};
            for (const value_t& item : *maybe_set)
            {
                res.insert(do_eval(item, stack));
            }
            return res;
        }
        else if (auto maybe_map = value.if_map())
        {
            map_t res = {};
            for (const auto& [key, val] : *maybe_map)
            {
                res.emplace(do_eval(key, stack), do_eval(val, stack));
            }
            return res;
        }
        return value;
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
                for (const value_t& v : *parameters.if_vector())
                {
                    const symbol_t& s = *v.if_symbol();
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
        stack_t::captured_t captured;

        auto operator()(const std::vector<value_t>& args) -> value_t
        {
            for (const overload_t& overload : overloads)
            {
                const auto [mandatory, maybe_variadic] = overload.params();

                if (args.size() == mandatory.size() && !maybe_variadic)
                {
                    stack_t::frame_type frame;
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        frame.emplace(mandatory.at(i), args.at(i));
                    }
                    stack_t new_stack{ std::move(frame), captured };
                    return self.eval_block(overload.body, new_stack);
                }
                if (args.size() > mandatory.size() && maybe_variadic)
                {
                    stack_t::frame_type frame;
                    list_t tail;
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        if (i < mandatory.size())
                        {
                            frame.emplace(mandatory.at(i), args.at(i));
                        }
                        else
                        {
                            tail.push_back(args.at(i));
                        }
                    }
                    frame.emplace(*maybe_variadic, std::move(tail));
                    stack_t new_stack{ std::move(frame), captured };
                    return self.eval_block(overload.body, new_stack);
                }
            }
            throw std::runtime_error{ str("could not resolve function overload for ", args.size(), " arg(s)") };
        };
    };

    auto eval_block(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return do_eval(item, stack); });
    }

    auto eval_let(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        const auto& bindings = *input.at(0).if_vector();
        stack_t current = stack;
        for (std::size_t i = 0; i < bindings.size(); i += 2)
        {
            value_t value = do_eval(bindings.at(i + 1), current);
            current = stack_t{ stack_t::frame_type{ { *bindings.at(i + 0).if_symbol(), std::move(value) } }, &current };
        }
        return eval_block({ input.begin() + 1, input.end() }, current);
    }

    auto eval_def(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return stack.define(*input.at(0).if_symbol(), do_eval(input.at(1), stack));
    }

    static auto create_overload(const std::vector<value_t>& input) -> clojure_t::overload_t
    {
        value_t parameters = *input.at(0).if_vector();
        return clojure_t::overload_t{ std::move(parameters), { input.begin() + 1, input.end() } };
    }

    auto eval_callable(const std::vector<value_t>& input, stack_t& stack) const -> callable_t
    {
        std::vector<clojure_t::overload_t> overloads;
        if (std::all_of(input.begin(), input.end(), [](const value_t& v) { return v.if_list(); }))
        {
            for (const value_t& v : input)
            {
                overloads.push_back(create_overload(*v.if_list()));
            }
        }
        else
        {
            overloads.push_back(create_overload(input));
        }
        return callable_t{ clojure_t{ *this, std::move(overloads), stack.capture() } };
    }

    auto eval_fn(const std::vector<value_t>& input, stack_t& stack) const -> value_t { return eval_callable(input, stack); }

    auto eval_defn(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return stack.define(*input.at(0).if_symbol(), eval_callable({ input.begin() + 1, input.end() }, stack));
    }

    auto eval_boolean(const value_t& value, stack_t& stack) const -> bool
    {
        return evaluate_detail::is_truthy(do_eval(value, stack));
    }

    auto eval_and(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        value_t result = true;
        for (const value_t& item : input)
        {
            result = do_eval(item, stack);
            if (!evaluate_detail::is_truthy(result))
            {
                break;
            }
        }
        return result;
    }

    auto eval_or(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        value_t result = nil;
        for (const value_t& item : input)
        {
            result = do_eval(item, stack);
            if (evaluate_detail::is_truthy(result))
            {
                break;
            }
        }
        return result;
    }

    auto eval_if(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return eval_boolean(input.at(0), stack) ? do_eval(input.at(1), stack) : do_eval(input.at(2), stack);
    }

    auto eval_cond(const std::vector<value_t>& input, stack_t& stack) const -> value_t
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

    auto eval_callable(const value_t& head, const std::vector<value_t>& tail, stack_t& stack) const -> value_t
    {
        const callable_t callable = evaluate_detail::as_callable(do_eval(head, stack), "callable");
        std::vector<value_t> args;
        args.reserve(tail.size());
        for (const value_t& item : tail)
        {
            args.push_back(do_eval(item, stack));
        }
        return callable(args);
    }

    auto eval_quote(const std::vector<value_t>& input, stack_t&) const -> value_t { return input[0]; }

    auto eval_thread_last(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        if (input.empty())
        {
            throw std::invalid_argument{ "`->>` requires an initial expression" };
        }
        value_t expression = input.front();
        for (auto it = input.begin() + 1; it != input.end(); ++it)
        {
            list_t call;
            if (const auto list = it->if_list())
            {
                call = *list;
            }
            else
            {
                call.push_back(*it);
            }
            call.push_back(std::move(expression));
            expression = std::move(call);
        }
        return do_eval(expression, stack);
    }

    auto eval_list(const list_t& input, stack_t& stack) const -> value_t
    {
        if (input.empty())
        {
            return nil;
        }
        const value_t& head = input.at(0);
        const auto tail = std::vector<value_t>(input.begin() + 1, input.end());

        using handler_t = value_t (evaluate_fn::*)(const std::vector<value_t>&, stack_t&) const;

        static const std::map<symbol_t, handler_t> handlers = {
            { symbol_t{ "quote" }, &evaluate_fn::eval_quote },      //
            { symbol_t{ "let" }, &evaluate_fn::eval_let },          //
            { symbol_t{ "def" }, &evaluate_fn::eval_def },          //
            { symbol_t{ "fn" }, &evaluate_fn::eval_fn },            //
            { symbol_t{ "defn" }, &evaluate_fn::eval_defn },        //
            { symbol_t{ "if" }, &evaluate_fn::eval_if },            //
            { symbol_t{ "cond" }, &evaluate_fn::eval_cond },        //
            { symbol_t{ "do" }, &evaluate_fn::eval_do },            //
            { symbol_t{ "and" }, &evaluate_fn::eval_and },          //
            { symbol_t{ "or" }, &evaluate_fn::eval_or },            //
            { symbol_t{ "->>" }, &evaluate_fn::eval_thread_last },  //
        };

        if (const auto h = head.if_symbol())
        {
            if (const auto handler = handlers.find(*h); handler != handlers.end())
            {
                return std::invoke(handler->second, this, tail, stack);
            }
        }
        return eval_callable(head, tail, stack);
    }

    auto eval_do(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return do_eval(item, stack); });
    }
} evaluate{};

}  // namespace edn
