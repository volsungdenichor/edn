#pragma once

#include <evaluate.hpp>
#include <parse.hpp>
#include <value.hpp>

namespace edn
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

}  // namespace edn
