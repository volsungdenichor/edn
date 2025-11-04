#pragma once

#include <iostream>
#include <parsing.hpp>

namespace edn
{

enum class token_type
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

inline std::ostream& operator<<(std::ostream& os, const token_type item)
{
    switch (item)
    {
        case token_type::quoted_string: return os << "quoted_string";
        case token_type::integer: return os << "integer";
        case token_type::floating_point: return os << "floating_point";
        case token_type::character: return os << "character";
        case token_type::keyword: return os << "keyword";
        case token_type::symbol: return os << "symbol";
        case token_type::parenthesis: return os << "parenthesis";
        case token_type::hash: return os << "hash";
        case token_type::quote: return os << "quote";
        default: break;
    }
    return os;
}

using token_t = std::pair<parsing::token_t, token_type>;

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

    static const inline auto parsers = std::vector<std::pair<parsing::parser_t, std::optional<token_type>>>{
        { space, std::nullopt },
        { comment, std::nullopt },
        { hash, token_type::hash },
        { quote, token_type::quote },
        { floating_point, token_type::floating_point },
        { integer, token_type::integer },
        { quoted_string, token_type::quoted_string },
        { character, token_type::character },
        { keyword, token_type::keyword },
        { symbol, token_type::symbol },
        { parenthesis, token_type::parenthesis },
    };
} tokenize{};

}  // namespace edn