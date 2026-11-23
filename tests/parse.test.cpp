#include <gmock/gmock.h>

#include <edn/edn.hpp>

#include "matchers.hpp"

TEST(parse, integer)
{
    EXPECT_THAT(edn::parse("42"), IsInteger(42));
}

TEST(parse, character)
{
    EXPECT_THAT(edn::parse(R"(\b)"), IsCharacter('b'));
    EXPECT_THAT(edn::parse(R"(\space)"), IsCharacter(' '));
}

TEST(parse, string)
{
    EXPECT_THAT(edn::parse(R"("Hello, World!")"), IsString(testing::StrEq("Hello, World!")));
}

TEST(parse, empty_set)
{
    for (const auto input : { "#{}", "#{ }", "#{\n; comment\n}" })
    {
        SCOPED_TRACE(input);
        EXPECT_THAT(edn::parse(input), IsSet(testing::IsEmpty()));
    }
}

TEST(parse, set_preserves_first_element_without_leading_whitespace)
{
    EXPECT_THAT(edn::parse("#{123}"), IsSet(testing::ElementsAre(IsInteger(123))));
    EXPECT_THAT(edn::parse("#{-123}"), IsSet(testing::ElementsAre(IsInteger(-123))));
    EXPECT_THAT(edn::parse("#{true}"), IsSet(testing::ElementsAre(IsBoolean(true))));
    EXPECT_THAT(edn::parse(R"(#{"text"})"), IsSet(testing::ElementsAre(IsString(testing::StrEq("text")))));
    EXPECT_THAT(edn::parse("#{1 2 1}"), IsSet(testing::UnorderedElementsAre(IsInteger(1), IsInteger(2))));
    EXPECT_THAT(edn::parse("#{ 1 2 }"), IsSet(testing::UnorderedElementsAre(IsInteger(1), IsInteger(2))));
}

TEST(parse, nested_sets_and_following_collection_elements)
{
    EXPECT_THAT(edn::parse("#{#{}}"), IsSet(testing::ElementsAre(IsSet(testing::IsEmpty()))));
    EXPECT_THAT(edn::parse("#{[1]}"), IsSet(testing::ElementsAre(IsVector(testing::ElementsAre(IsInteger(1))))));
    EXPECT_THAT(
        edn::parse("[#{} #{123} 4]"),
        IsVector(
            testing::ElementsAre(IsSet(testing::IsEmpty()), IsSet(testing::ElementsAre(IsInteger(123))), IsInteger(4))));
}

TEST(parse, set_requires_a_closing_delimiter)
{
    for (const auto input : { "#{", "#{123", "#{#{}" })
    {
        SCOPED_TRACE(input);
        EXPECT_THAT(
            [&]() { return edn::parse(input); },
            testing::ThrowsMessage<edn::parse_error>(testing::HasSubstr("Unterminated set")));
    }
    EXPECT_THAT(
        []() { return edn::parse("#{]"); },
        testing::ThrowsMessage<edn::parse_error>(testing::HasSubstr("Unexpected closing delimiter")));
}

TEST(parse, tagged_element_with_space)
{
    EXPECT_THAT(
        edn::parse(R"(#inst "2024-01-01")"),
        IsTaggedElement(
            testing::AllOf(TagIs(testing::StrEq("inst")), TaggedElementIs(IsString(testing::StrEq("2024-01-01"))))));
}

TEST(parse, tagged_element_without_space)
{
    EXPECT_THAT(
        edn::parse(R"(#inst"2024-01-01")"),
        IsTaggedElement(
            testing::AllOf(TagIs(testing::StrEq("inst")), TaggedElementIs(IsString(testing::StrEq("2024-01-01"))))));
}
