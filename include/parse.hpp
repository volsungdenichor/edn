#pragma once

#include <value.hpp>

#include "tokenize.hpp"

namespace edn
{

constexpr inline struct parse_fn
{
    auto operator()(std::string_view text) const -> value_t
    {
        std::vector<token_t> tokens = tokenize(text);

        std::vector<value_t> values;
        while (!tokens.empty())
        {
            values.push_back(read_from(tokens));
        }
        if (values.empty())
        {
            return nil_t{};
        }
        else if (values.size() == 1)
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
            throw std::runtime_error{ "Cannot pop from empty vector" };
        }
        T result = v.front();
        v.erase(std::begin(v));
        return result;
    }

    static auto read_from(std::vector<token_t>& tokens) -> value_t
    {
        if (tokens.empty())
        {
            return value_t();
        }
        const token_t token_info = pop_front(tokens);
        const auto& [token, token_type] = token_info;
        if (token_type == token_type_t::quote)
        {
            return quoted_element_t{ read_from(tokens) };
        }
        if (token.value == "(")
        {
            const std::vector<value_t> items = read_until(tokens, ")");
            return list_t{ items.begin(), items.end() };
        }
        else if (token.value == "[")
        {
            const std::vector<value_t> items = read_until(tokens, "]");
            return vector_t{ items.begin(), items.end() };
        }
        else if (token_type == token_type_t::hash)
        {
            const auto& [next_token, next_token_type] = pop_front(tokens);
            if (next_token.value == "{")
            {
                const std::vector<value_t> items = read_until(tokens, "}");
                return set_t{ items.begin(), items.end() };
            }
            else
            {
                return tagged_element_t{ symbol_t{ next_token.value.c_str() }, read_from(tokens) };
            }
        }
        else if (token.value == "{")
        {
            return to_map(read_until(tokens, "}"));
        }
        return read_atom(token_info);
    }

    static auto read_until(std::vector<token_t>& tokens, const std::string& delimiter) -> std::vector<value_t>
    {
        auto result = std::vector<value_t>{};
        if (tokens.empty())
        {
            throw std::runtime_error{ "invalid parentheses" };
        }
        while (!tokens.empty() && tokens.front().first.value != delimiter)
        {
            value_t v = read_from(tokens);
            result.push_back(std::move(v));
        }
        pop_front(tokens);
        return result;
    }

    static auto to_map(const std::vector<value_t>& items) -> map_t
    {
        if (items.size() % 2 != 0)
        {
            throw std::runtime_error{ "Map expects to have even number of elements" };
        }
        auto result = map_t();
        for (std::size_t i = 0; i < items.size(); i += 2)
        {
            result.emplace(items[i + 0], items[i + 1]);
        }
        return result;
    }

    template <class T>
    static auto parse(const std::string& text) -> T
    {
        T value = {};
        std::stringstream ss;
        ss << text;
        ss >> value;
        return value;
    }

    static auto read_atom(const token_t& token_info) -> value_t
    {
        const auto& [token, token_type] = token_info;
        if (token_type == token_type_t::integer)
        {
            return integer_t{ parse<int>(token.value) };
        }
        else if (token_type == token_type_t::floating_point)
        {
            return floating_point_t{ parse<double>(token.value) };
        }
        else if (token_type == token_type_t::character)
        {
            if (token.value.size() == 1 && std::isprint(token.value[0]))
            {
                return character_t{ token.value[0] };
            }
            for (const auto& [symbol, name] : character_names)
            {
                if (token.value == name)
                {
                    return character_t{ symbol };
                }
            }
            throw std::runtime_error{ str("Unhandled character ", token.value) };
        }
        else if (token_type == token_type_t::keyword)
        {
            return keyword_t{ token.value.c_str() };
        }
        else if (token_type == token_type_t::quoted_string)
        {
            return string_t{ token.value };
        }
        static const auto registered_words = std::map<std::string, value_t>{
            { "nil", nil_t{} },
            { "true", boolean_t{ true } },
            { "false", boolean_t{ false } },
        };
        if (const auto word = registered_words.find(token.value); word != registered_words.end())
        {
            return word->second;
        }
        return symbol_t{ token.value.c_str() };
    }
} parse{};

}  // namespace edn