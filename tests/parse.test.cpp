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
