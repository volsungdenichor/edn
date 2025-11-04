#pragma once

#include <iostream>
#include <parsing.hpp>

namespace edn
{

enum class token_type_t
{
    quoted_string,
    integer,
    floating_point,
    character,
    keyword,
    symbol,
    parenthesis,
    hash,
    quote
};

inline std::ostream& operator<<(std::ostream& os, const token_type_t item)
{
    switch (item)
    {
        case token_type_t::quoted_string: return os << "quoted_string";
        case token_type_t::integer: return os << "integer";
        case token_type_t::floating_point: return os << "floating_point";
        case token_type_t::character: return os << "character";
        case token_type_t::keyword: return os << "keyword";
        case token_type_t::symbol: return os << "symbol";
        case token_type_t::parenthesis: return os << "parenthesis";
        case token_type_t::hash: return os << "hash";
        case token_type_t::quote: return os << "quote";
        default: break;
    }
    return os;
}

using token_t = std::pair<parsing::token_t, token_type_t>;

constexpr inline struct tokenizer_fn
{
    auto operator()(std::string_view text) const -> std::vector<token_t>
    {
        auto tokens = std::vector<token_t>{};
        auto stream = parsing::stream_t{ text };
        while (stream)
        {
            for (const auto& [parser, token_type] : parsers)
            {
                if (parsing::maybe_parse_result_t res = parser(stream))
                {
                    stream = res->second;
                    if (token_type)
                    {
                        tokens.emplace_back(res->first, *token_type);
                    }
                }
            }
        }
        return tokens;
    }

private:
    static const inline auto is_parenthesis = parsing::one_of("()[]{}");
    static const inline auto hash = parsing::character('#');
    static const inline auto parenthesis = parsing::character(is_parenthesis);
    static const inline auto comment = parsing::character(';') >> parsing::many(parsing::character(parsing::ne('\n')));
    static const inline auto quoted_string = parsing::quoted_string();
    static const inline auto symbol = parsing::sequence(
        parsing::character([](char ch) { return !is_parenthesis(ch) && !parsing::is_space(ch) && !parsing::is_digit(ch); }),
        parsing::many(parsing::character([](char ch) { return !is_parenthesis(ch) && !parsing::is_space(ch); })));
    static const inline auto keyword = parsing::character(':') >> symbol;
    static const inline auto character
        = parsing::character('\\')
          >> parsing::at_least(1)(parsing::character([](char ch) { return parsing::is_graph(ch) && !is_parenthesis(ch); }));
    static const inline auto space = parsing::character([](char ch) { return parsing::is_space(ch) || ch == ','; });
    static const inline auto integer = parsing::sequence(
        parsing::optional(parsing::any(parsing::character('+'), parsing::character('-'))),
        parsing::at_least(1)(parsing::digit));
    static const inline auto floating_point = parsing::sequence(
        parsing::optional(parsing::any(parsing::character('+'), parsing::character('-'))),
        parsing::at_least(1)(parsing::digit),
        parsing::character('.'),
        parsing::many(parsing::digit));
    static const inline auto quote = parsing::character('\'');

    static const inline auto parsers = std::vector<std::pair<parsing::parser_t, std::optional<token_type_t>>>{
        { space, std::nullopt },
        { comment, std::nullopt },
        { hash, token_type_t::hash },
        { quote, token_type_t::quote },
        { floating_point, token_type_t::floating_point },
        { integer, token_type_t::integer },
        { quoted_string, token_type_t::quoted_string },
        { character, token_type_t::character },
        { keyword, token_type_t::keyword },
        { symbol, token_type_t::symbol },
        { parenthesis, token_type_t::parenthesis },
    };
} tokenize{};

}  // namespace edn