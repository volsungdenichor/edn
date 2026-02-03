#include <gmock/gmock.h>

#include <edn/edn.hpp>

#include "matchers.hpp"

TEST(parse, integer)
{
    EXPECT_THAT(
        edn::parse("42"), testing::AllOf(testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(42))));
}

TEST(parse, character)
{
    EXPECT_THAT(
        edn::parse(R"(\b)"),
        testing::AllOf(testing::Property("if_character", &edn::value_t::if_character, testing::Pointee('b'))));
    EXPECT_THAT(
        edn::parse(R"(\space)"),
        testing::AllOf(testing::Property("if_character", &edn::value_t::if_character, testing::Pointee(' '))));
}

TEST(parse, string)
{
    EXPECT_THAT(
        edn::parse(R"("Hello, World!")"),
        testing::AllOf(
            testing::Property("if_string", &edn::value_t::if_string, testing::Pointee(testing::StrEq("Hello, World!")))));
}

TEST(parse, tagged_element_with_space)
{
    EXPECT_THAT(
        edn::parse(R"(#inst "2024-01-01")"),
        testing::AllOf(
            IsTaggedElement(testing::AllOf(
                TagIs(testing::StrEq("inst")),
                TaggedElementIs(testing::AllOf(
                    IsString(testing::StrEq("2024-01-01")),
                    OfType(edn::value_type_t::string))))),
            OfType(edn::value_type_t::tagged_element)));
}

TEST(parse, tagged_element_without_space)
{
    EXPECT_THAT(
        edn::parse(R"(#inst"2024-01-01")"),
        testing::AllOf(
            IsTaggedElement(testing::AllOf(
                TagIs(testing::StrEq("inst")),
                TaggedElementIs(testing::AllOf(
                    IsString(testing::StrEq("2024-01-01")),
                    OfType(edn::value_type_t::string))))),
            OfType(edn::value_type_t::tagged_element)));
}
