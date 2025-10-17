#include <edn.hpp>
#include <fstream>
#include <iostream>
#include <memory>

namespace edn
{

template <class E>
struct enum_codec
{
    std::vector<std::pair<E, keyword_t>> m_values;

    explicit enum_codec(std::vector<std::pair<E, keyword_t>> values) : m_values(std::move(values))
    {
    }

    value_t encode(E in) const
    {
        for (const auto& pair : m_values)
        {
            if (pair.first == in)
            {
                return keyword_t{ pair.second };
            }
        }
        throw std::runtime_error{ edn::str("On encoding enum: unregistered value") };
    }

    E decode(const value_t& in) const
    {
        const auto& k = as<keyword_t>(in);
        for (const auto& pair : m_values)
        {
            if (pair.second == k)
            {
                return pair.first;
            }
        }
        throw std::runtime_error{ str("On decoding enum: unknown value '", k, "'") };
    }
};

template <class T>
struct struct_codec
{
    struct field_info
    {
        keyword_t name;
        std::function<void(map_t&, const T&, const keyword_t&)> encode;
        std::function<void(T&, const map_t&, const keyword_t&)> decode;

        template <class Type>
        field_info(Type T::*field, keyword_t n)
            : name(std::move(n))
            , encode{ [=](map_t& out, const T& in, const keyword_t& n) { out.emplace(n, edn::encode(in.*field)); } }
            , decode{ [=](T& out, const map_t& in, const keyword_t& n) { edn::decode(out.*field, in.at(n)); } }
        {
        }
    };

    std::vector<field_info> m_fields;

    explicit struct_codec(std::vector<field_info> fields) : m_fields(std::move(fields))
    {
    }

    value_t encode(const T& in) const
    {
        map_t out = {};
        for (const field_info& field : m_fields)
        {
            try
            {
                field.encode(out, in, field.name);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error{ str("On encoding field '", field.name, "': ", ex.what()) };
            }
        }
        return out;
    }

    T decode(const value_t& in) const
    {
        static_assert(std::is_default_constructible_v<T>, "Default constructible type required");
        const auto& m = as<map_t>(in);
        T out = {};
        for (const field_info& field : m_fields)
        {
            try
            {
                field.decode(out, m, field.name);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error{ str("On decoding field '", field.name, "': ", ex.what()) };
            }
        }
        return out;
    }
};

template <class T>
struct codec<std::vector<T>>
{
    value_t encode(const std::vector<T>& in) const
    {
        vector_t out = {};
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(edn::encode(item));
        }
        return out;
    }

    std::vector<T> decode(const value_t& in) const
    {
        const auto& a = as<vector_t>(in);
        std::vector<T> out = {};
        out.reserve(a.size());
        for (const value_t& v : a)
        {
            out.push_back(edn::decode<T>(v));
        }
        return out;
    }
};

template <>
struct codec<std::string>
{
    value_t encode(const std::string& in) const
    {
        return string_t{ in.c_str() };
    }

    std::string decode(const value_t& in) const
    {
        return std::string{ as<string_t>(in).c_str() };
    }
};

}  // namespace edn

enum class suit
{
    heart,
    diamond,
    club,
    spade
};

enum class rank
{
    jack,
    queen,
    king,
    ace
};

struct card
{
    enum suit suit;
    enum rank rank;
};

template <>
struct edn::codec<suit> : enum_codec<suit>
{
    codec()
        : enum_codec(
            { { suit::heart, "heart" }, { suit::diamond, "diamond" }, { suit::club, "club" }, { suit::spade, "spade" } })
    {
    }
};

template <>
struct edn::codec<rank> : enum_codec<rank>
{
    codec() : enum_codec({ { rank::jack, "jack" }, { rank::queen, "queen" }, { rank::king, "king" }, { rank::ace, "ace" } })
    {
    }
};

template <>
struct edn::codec<card> : struct_codec<card>
{
    codec() : struct_codec({ { &card::rank, "rank" }, { &card::suit, "suit" } })
    {
    }
};

void print(std::ostream& os, const edn::value_t& item, std::size_t level = 0)
{
    const auto tab = std::string(level * 2, ' ');
    std::visit(
        edn::overloaded{ [&](const edn::vector_t& v)
                         {
                             os << tab << '[' << "\n";
                             for (const edn::value_t& it : v)
                             {
                                 print(os, it, level + 1);
                             }
                             os << tab << ']' << "\n";
                         },
                         [&](const edn::map_t& v)
                         {
                             os << tab << '{' << "\n";
                             for (const auto& it : v)
                             {
                                 print(os, it.first, level + 1);
                                 print(os, it.second, level + 1);
                             }
                             os << tab << '}' << "\n";
                         },
                         [&](const auto& v) { os << tab << v; }

        },
        item);
    os << "\n";
}

void run(const std::vector<std::string>& args)
{
    print(
        std::cout,
        edn::parse(" [#person {:first-name \"Adam\" :last-name \"Mickiewicz\"} #person {:first-name \"Juliuesz\" :last-name "
                   "\"Slowacki\"}]"));
}

int main(int argc, char** argv)
{
    try
    {
        run(std::vector<std::string>(argv, argv + argc));
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nError:\n" << ex.what() << "\n";
    }
}
