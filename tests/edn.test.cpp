#include <gmock/gmock.h>

#include <edn/edn.hpp>

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

TEST(edn, default_construced_value_is_nil)
{
    EXPECT_THAT(
        edn::value_t{},
        testing::AllOf(
            testing::Property("is_nil", &edn::value_t::is_nil, true),
            OfType(edn::value_type_t::nil),
            WhenSerialized(testing::StrEq("nil"))));
}

TEST(edn, nil)
{
    EXPECT_THAT(
        edn::value_t{ edn::nil },
        testing::AllOf(
            testing::Property("is_nil", &edn::value_t::is_nil, true),
            OfType(edn::value_type_t::nil),
            WhenSerialized(testing::StrEq("nil"))));
}

TEST(edn, integer)
{
    EXPECT_THAT(
        edn::value_t{ 42 },
        testing::AllOf(
            testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(42)),
            OfType(edn::value_type_t::integer),
            WhenSerialized(testing::StrEq("42"))));
}

TEST(edn, floating_point)
{
    EXPECT_THAT(
        edn::value_t{ 2.71828 },
        testing::AllOf(
            testing::Property("if_floating_point", &edn::value_t::if_floating_point, testing::Pointee(2.71828)),
            OfType(edn::value_type_t::floating_point),
            WhenSerialized(testing::StrEq("2.71828"))));
}

TEST(edn, boolean)
{
    EXPECT_THAT(
        edn::value_t{ true },
        testing::AllOf(
            testing::Property("if_boolean", &edn::value_t::if_boolean, testing::Pointee(true)),
            OfType(edn::value_type_t::boolean),
            WhenSerialized(testing::StrEq("true"))));
}

TEST(edn, character)
{
    EXPECT_THAT(
        edn::value_t{ ' ' },
        testing::AllOf(
            testing::Property("if_character", &edn::value_t::if_character, testing::Pointee(' ')),
            OfType(edn::value_type_t::character),
            WhenSerialized(testing::StrEq(R"(\space)"))));
}

TEST(edn, string)
{
    EXPECT_THAT(
        edn::value_t{ std::string{ "Benvenuto" } },
        testing::AllOf(
            testing::Property("if_string", &edn::value_t::if_string, testing::Pointee(testing::StrEq("Benvenuto"))),
            OfType(edn::value_type_t::string),
            WhenSerialized(testing::StrEq(R"("Benvenuto")"))));
}

TEST(edn, symbol)
{
    EXPECT_THAT(
        edn::value_t{ edn::symbol_t{ "my-symbol" } },
        testing::AllOf(
            testing::Property("if_symbol", &edn::value_t::if_symbol, testing::Pointee(testing::StrEq("my-symbol"))),
            OfType(edn::value_type_t::symbol),
            WhenSerialized(testing::StrEq("my-symbol"))));
}

TEST(edn, keyword)
{
    EXPECT_THAT(
        edn::value_t{ edn::keyword_t{ "my-keyword" } },
        testing::AllOf(
            testing::Property("if_keyword", &edn::value_t::if_keyword, testing::Pointee(testing::StrEq("my-keyword"))),
            OfType(edn::value_type_t::keyword),
            WhenSerialized(testing::StrEq(":my-keyword"))));
}

TEST(edn, tagged_element)
{
    EXPECT_THAT(
        (edn::value_t{ edn::tagged_element_t{ edn::symbol_t{ "inst" }, edn::string_t{ "2024-01-01" } } }),
        testing::AllOf(
            testing::Property(
                "if_tagged_element",
                &edn::value_t::if_tagged_element,
                testing::Pointee(testing::AllOf(
                    testing::Property("tag", &edn::tagged_element_t::tag, testing::StrEq("inst")),
                    testing::Property(
                        "element",
                        &edn::tagged_element_t::element,
                        testing::AllOf(
                            testing::Property(
                                "if_string", &edn::value_t::if_string, testing::Pointee(testing::StrEq("2024-01-01"))),
                            OfType(edn::value_type_t::string),
                            WhenSerialized(testing::StrEq(R"("2024-01-01")"))))))),
            OfType(edn::value_type_t::tagged_element),
            WhenSerialized(testing::StrEq(R"(#inst "2024-01-01")"))));
}

TEST(edn, quoted_element)
{
    EXPECT_THAT(
        (edn::value_t{ edn::quoted_element_t{ edn::vector_t{ 1, 2, 3 } } }),
        testing::AllOf(
            testing::Property(
                "if_quoted_element",
                &edn::value_t::if_quoted_element,
                testing::Pointee(testing::AllOf(testing::Property(
                    "element",
                    &edn::quoted_element_t::element,
                    testing::AllOf(
                        testing::Property(
                            "if_vector",
                            &edn::value_t::if_vector,
                            testing::Pointee(testing::ElementsAre(
                                testing::AllOf(
                                    testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(1)),
                                    OfType(edn::value_type_t::integer),
                                    WhenSerialized(testing::StrEq("1"))),
                                testing::AllOf(
                                    testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(2)),
                                    OfType(edn::value_type_t::integer),
                                    WhenSerialized(testing::StrEq("2"))),
                                testing::AllOf(
                                    testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(3)),
                                    OfType(edn::value_type_t::integer),
                                    WhenSerialized(testing::StrEq("3")))))),
                        OfType(edn::value_type_t::vector),
                        WhenSerialized(testing::StrEq("[1 2 3]"))))))),
            OfType(edn::value_type_t::quoted_element),
            WhenSerialized(testing::StrEq("'[1 2 3]"))));
}

TEST(edn, vector)
{
    EXPECT_THAT(
        (edn::value_t{ edn::vector_t{ 1, "A", 'a' } }),
        testing::AllOf(
            testing::Property(
                "if_vector",
                &edn::value_t::if_vector,
                testing::Pointee(testing::ElementsAre(
                    testing::AllOf(
                        testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(1)),
                        OfType(edn::value_type_t::integer),
                        WhenSerialized(testing::StrEq("1"))),
                    testing::AllOf(
                        testing::Property("if_string", &edn::value_t::if_string, testing::Pointee(testing::StrEq("A"))),
                        OfType(edn::value_type_t::string),
                        WhenSerialized(testing::StrEq(R"("A")"))),
                    testing::AllOf(
                        testing::Property("if_character", &edn::value_t::if_character, testing::Pointee('a')),
                        OfType(edn::value_type_t::character),
                        WhenSerialized(testing::StrEq(R"(\a)")))))),
            OfType(edn::value_type_t::vector),
            WhenSerialized(testing::StrEq(R"([1 "A" \a])"))));
}

TEST(edn, list)
{
    EXPECT_THAT(
        (edn::value_t{ edn::list_t{ edn::symbol_t{ "+" }, 1, 2 } }),
        testing::AllOf(
            testing::Property(
                "if_list",
                &edn::value_t::if_list,
                testing::Pointee(testing::ElementsAre(
                    testing::AllOf(
                        testing::Property("if_symbol", &edn::value_t::if_symbol, testing::Pointee(edn::symbol_t{ "+" })),
                        OfType(edn::value_type_t::symbol),
                        WhenSerialized(testing::StrEq("+"))),
                    testing::AllOf(
                        testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(1)),
                        OfType(edn::value_type_t::integer),
                        WhenSerialized(testing::StrEq("1"))),
                    testing::AllOf(
                        testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(2)),
                        OfType(edn::value_type_t::integer),
                        WhenSerialized(testing::StrEq("2")))))),
            OfType(edn::value_type_t::list),
            WhenSerialized(testing::StrEq(R"((+ 1 2))"))));
}

TEST(edn, set)
{
    EXPECT_THAT(
        (edn::value_t{ edn::set_t{ 1, 2, 3 } }),
        testing::AllOf(
            testing::Property(
                "if_set",
                &edn::value_t::if_set,
                testing::Pointee(testing::UnorderedElementsAre(
                    testing::AllOf(
                        testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(1)),
                        OfType(edn::value_type_t::integer),
                        WhenSerialized(testing::StrEq("1"))),
                    testing::AllOf(
                        testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(2)),
                        OfType(edn::value_type_t::integer),
                        WhenSerialized(testing::StrEq("2"))),
                    testing::AllOf(
                        testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(3)),
                        OfType(edn::value_type_t::integer),
                        WhenSerialized(testing::StrEq("3")))))),
            OfType(edn::value_type_t::set),
            WhenSerialized(testing::StrEq("#{1 2 3}"))));
}

TEST(edn, map)
{
    EXPECT_THAT(
        (edn::value_t{
            edn::map_t{ { edn::keyword_t{ "name" }, edn::string_t{ "John" } }, { edn::keyword_t{ "age" }, 30 } } }),
        testing::AllOf(
            testing::Property(
                "if_map",
                &edn::value_t::if_map,
                testing::Pointee(testing::UnorderedElementsAre(
                    testing::Pair(
                        testing::AllOf(
                            testing::Property(
                                "if_keyword", &edn::value_t::if_keyword, testing::Pointee(testing::StrEq("name"))),
                            OfType(edn::value_type_t::keyword)),
                        testing::AllOf(
                            testing::Property(
                                "if_string", &edn::value_t::if_string, testing::Pointee(testing::StrEq("John"))),
                            OfType(edn::value_type_t::string),
                            WhenSerialized(testing::StrEq(R"("John")")))),
                    testing::Pair(
                        testing::AllOf(
                            testing::Property(
                                "if_keyword", &edn::value_t::if_keyword, testing::Pointee(testing::StrEq("age"))),
                            OfType(edn::value_type_t::keyword)),
                        testing::AllOf(
                            testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(30)),
                            OfType(edn::value_type_t::integer),
                            WhenSerialized(testing::StrEq("30"))))))),
            OfType(edn::value_type_t::map),
            WhenSerialized(testing::StrEq(R"({:age 30 :name "John"})"))));
}
