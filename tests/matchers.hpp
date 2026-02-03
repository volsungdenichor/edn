#pragma once

#include <gmock/gmock.h>

#include <edn/edn.hpp>
#include <sstream>

constexpr auto WhenSerialized = [](auto&& matcher)
{
    return testing::ResultOf(
        "serialized",
        [](const auto& value)
        {
            std::ostringstream os;
            os << value;
            return os.str();
        },
        std::forward<decltype(matcher)>(matcher));
};

constexpr auto OfType = [](auto&& matcher)
{ return testing::Property("type", &edn::value_t::type, std::forward<decltype(matcher)>(matcher)); };

constexpr auto IsNil = [](auto&& matcher)
{ return testing::Property("is_nil", &edn::value_t::is_nil, std::forward<decltype(matcher)>(matcher)); };

constexpr auto IsInteger = [](auto&& matcher)
{
    return testing::Property(
        "if_integer", &edn::value_t::if_integer, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsFloatingPoint = [](auto&& matcher)
{
    return testing::Property(
        "if_floating_point", &edn::value_t::if_floating_point, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsBoolean = [](auto&& matcher)
{
    return testing::Property(
        "if_boolean", &edn::value_t::if_boolean, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsCharacter = [](auto&& matcher)
{
    return testing::Property(
        "if_character", &edn::value_t::if_character, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsString = [](auto&& matcher)
{
    return testing::Property(
        "if_string", &edn::value_t::if_string, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsSymbol = [](auto&& matcher)
{
    return testing::Property(
        "if_symbol", &edn::value_t::if_symbol, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsKeyword = [](auto&& matcher)
{
    return testing::Property(
        "if_keyword", &edn::value_t::if_keyword, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsTaggedElement = [](auto&& matcher)
{
    return testing::Property(
        "if_tagged_element", &edn::value_t::if_tagged_element, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsQuotedElement = [](auto&& matcher)
{
    return testing::Property(
        "if_quoted_element", &edn::value_t::if_quoted_element, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsVector = [](auto&& matcher)
{
    return testing::Property(
        "if_vector", &edn::value_t::if_vector, testing::Pointee(std::forward<decltype(matcher)>(matcher)));
};

constexpr auto IsList = [](auto&& matcher)
{ return testing::Property("if_list", &edn::value_t::if_list, testing::Pointee(std::forward<decltype(matcher)>(matcher))); };

constexpr auto IsSet = [](auto&& matcher)
{ return testing::Property("if_set", &edn::value_t::if_set, testing::Pointee(std::forward<decltype(matcher)>(matcher))); };

constexpr auto IsMap = [](auto&& matcher)
{ return testing::Property("if_map", &edn::value_t::if_map, testing::Pointee(std::forward<decltype(matcher)>(matcher))); };

constexpr auto TagIs = [](auto&& matcher)
{ return testing::Property("tag", &edn::tagged_element_t::tag, std::forward<decltype(matcher)>(matcher)); };

constexpr auto TaggedElementIs = [](auto&& matcher)
{ return testing::Property("element", &edn::tagged_element_t::element, std::forward<decltype(matcher)>(matcher)); };

constexpr auto QuotedElementIs = [](auto&& matcher)
{ return testing::Property("element", &edn::quoted_element_t::element, std::forward<decltype(matcher)>(matcher)); };