#include <gmock/gmock.h>

#include <edn/edn.hpp>

#include "matchers.hpp"

TEST(edn, default_construced_value_is_nil)
{
    EXPECT_THAT(
        edn::value_t{}, testing::AllOf(IsNil(true), OfType(edn::value_type_t::nil), WhenSerialized(testing::StrEq("nil"))));
}

TEST(edn, nil)
{
    EXPECT_THAT(
        edn::value_t{ edn::nil },
        testing::AllOf(IsNil(true), OfType(edn::value_type_t::nil), WhenSerialized(testing::StrEq("nil"))));
}

TEST(edn, integer)
{
    EXPECT_THAT(
        edn::value_t{ 42 },
        testing::AllOf(IsInteger(42), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("42"))));
}

TEST(edn, floating_point)
{
    EXPECT_THAT(
        edn::value_t{ 2.71828 },
        testing::AllOf(
            IsFloatingPoint(2.71828), OfType(edn::value_type_t::floating_point), WhenSerialized(testing::StrEq("2.71828"))));
}

TEST(edn, boolean)
{
    EXPECT_THAT(
        edn::value_t{ true },
        testing::AllOf(IsBoolean(true), OfType(edn::value_type_t::boolean), WhenSerialized(testing::StrEq("true"))));
}

TEST(edn, character)
{
    EXPECT_THAT(
        edn::value_t{ ' ' },
        testing::AllOf(IsCharacter(' '), OfType(edn::value_type_t::character), WhenSerialized(testing::StrEq(R"(\space)"))));
}

TEST(edn, string)
{
    EXPECT_THAT(
        edn::value_t{ std::string{ "Benvenuto" } },
        testing::AllOf(
            IsString(testing::StrEq("Benvenuto")),
            OfType(edn::value_type_t::string),
            WhenSerialized(testing::StrEq(R"("Benvenuto")"))));
}

TEST(edn, symbol)
{
    EXPECT_THAT(
        edn::value_t{ edn::symbol_t{ "my-symbol" } },
        testing::AllOf(
            IsSymbol(testing::StrEq("my-symbol")),
            OfType(edn::value_type_t::symbol),
            WhenSerialized(testing::StrEq("my-symbol"))));
}

TEST(edn, keyword)
{
    EXPECT_THAT(
        edn::value_t{ edn::keyword_t{ "my-keyword" } },
        testing::AllOf(
            IsKeyword(testing::StrEq("my-keyword")),
            OfType(edn::value_type_t::keyword),
            WhenSerialized(testing::StrEq(":my-keyword"))));
}

TEST(edn, tagged_element)
{
    EXPECT_THAT(
        (edn::value_t{ edn::tagged_element_t{ edn::symbol_t{ "inst" }, edn::string_t{ "2024-01-01" } } }),
        testing::AllOf(
            IsTaggedElement(testing::AllOf(
                TagIs(testing::StrEq("inst")),
                TaggedElementIs(testing::AllOf(
                    IsString(testing::StrEq("2024-01-01")),
                    OfType(edn::value_type_t::string),
                    WhenSerialized(testing::StrEq(R"("2024-01-01")")))))),
            OfType(edn::value_type_t::tagged_element),
            WhenSerialized(testing::StrEq(R"(#inst "2024-01-01")"))));
}

TEST(edn, quoted_element)
{
    EXPECT_THAT(
        (edn::value_t{ edn::quoted_element_t{ edn::vector_t{ 1, 2, 3 } } }),
        testing::AllOf(
            IsQuotedElement(QuotedElementIs(testing::AllOf(
                IsVector(testing::ElementsAre(
                    testing::AllOf(IsInteger(1), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("1"))),
                    testing::AllOf(IsInteger(2), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("2"))),
                    testing::AllOf(IsInteger(3), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("3"))))),
                OfType(edn::value_type_t::vector),
                WhenSerialized(testing::StrEq("[1 2 3]"))))),
            OfType(edn::value_type_t::quoted_element),
            WhenSerialized(testing::StrEq("'[1 2 3]"))));
}

TEST(edn, vector)
{
    EXPECT_THAT(
        (edn::value_t{ edn::vector_t{ 1, "A", 'a' } }),
        testing::AllOf(
            IsVector(testing::ElementsAre(
                testing::AllOf(IsInteger(1), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("1"))),
                testing::AllOf(
                    IsString(testing::StrEq("A")),
                    OfType(edn::value_type_t::string),
                    WhenSerialized(testing::StrEq(R"("A")"))),
                testing::AllOf(
                    IsCharacter('a'), OfType(edn::value_type_t::character), WhenSerialized(testing::StrEq(R"(\a)"))))),
            OfType(edn::value_type_t::vector),
            WhenSerialized(testing::StrEq(R"([1 "A" \a])"))));
}

TEST(edn, list)
{
    EXPECT_THAT(
        (edn::value_t{ edn::list_t{ edn::symbol_t{ "+" }, 1, 2 } }),
        testing::AllOf(
            IsList(testing::ElementsAre(
                testing::AllOf(
                    IsSymbol(testing::StrEq("+")), OfType(edn::value_type_t::symbol), WhenSerialized(testing::StrEq("+"))),
                testing::AllOf(IsInteger(1), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("1"))),
                testing::AllOf(IsInteger(2), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("2"))))),
            OfType(edn::value_type_t::list),
            WhenSerialized(testing::StrEq(R"((+ 1 2))"))));
}

TEST(edn, set)
{
    EXPECT_THAT(
        (edn::value_t{ edn::set_t{ 1, 2, 3 } }),
        testing::AllOf(
            IsSet(testing::UnorderedElementsAre(
                testing::AllOf(IsInteger(1), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("1"))),
                testing::AllOf(IsInteger(2), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("2"))),
                testing::AllOf(IsInteger(3), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("3"))))),
            OfType(edn::value_type_t::set),
            WhenSerialized(testing::StrEq("#{1 2 3}"))));
}

TEST(edn, map)
{
    EXPECT_THAT(
        (edn::value_t{ edn::map_t{ { edn::keyword_t{ "name" }, "John" }, { edn::keyword_t{ "age" }, 30 } } }),
        testing::AllOf(
            IsMap(testing::ElementsAre(
                testing::Pair(
                    testing::AllOf(IsKeyword(testing::StrEq("name")), OfType(edn::value_type_t::keyword)),
                    testing::AllOf(
                        IsString(testing::StrEq("John")),
                        OfType(edn::value_type_t::string),
                        WhenSerialized(testing::StrEq(R"("John")")))),
                testing::Pair(
                    testing::AllOf(IsKeyword(testing::StrEq("age")), OfType(edn::value_type_t::keyword)),
                    testing::AllOf(
                        IsInteger(30), OfType(edn::value_type_t::integer), WhenSerialized(testing::StrEq("30")))))),
            OfType(edn::value_type_t::map),
            WhenSerialized(testing::StrEq(R"({:name "John" :age 30})"))));
}
