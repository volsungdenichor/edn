#include <gmock/gmock.h>

#include <edn/edn.hpp>

template <class Matcher>
constexpr auto SerializedIs(Matcher&& matcher)
{
    return testing::ResultOf(
        "serialized",
        [](const auto& value)
        {
            std::ostringstream os;
            os << value;
            return os.str();
        },
        std::forward<Matcher>(matcher));
}

TEST(edn, default_construced_value_is_nil)
{
    EXPECT_THAT(
        edn::value_t{},
        testing::AllOf(
            testing::Property("is_nil", &edn::value_t::is_nil, true),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::nil),
            SerializedIs(testing::StrEq("nil"))));
}

TEST(edn, integer)
{
    EXPECT_THAT(
        edn::value_t{ 42 },
        testing::AllOf(
            testing::Property("if_integer", &edn::value_t::if_integer, testing::Pointee(42)),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::integer),
            SerializedIs(testing::StrEq("42"))));
}

TEST(edn, floating_point)
{
    EXPECT_THAT(
        edn::value_t{ 2.71828 },
        testing::AllOf(
            testing::Property("if_floating_point", &edn::value_t::if_floating_point, testing::Pointee(2.71828)),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::floating_point),
            SerializedIs(testing::StrEq("2.71828"))));
}

TEST(edn, boolean)
{
    EXPECT_THAT(
        edn::value_t{ true },
        testing::AllOf(
            testing::Property("if_boolean", &edn::value_t::if_boolean, testing::Pointee(true)),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::boolean),
            SerializedIs(testing::StrEq("true"))));
}

TEST(edn, character)
{
    EXPECT_THAT(
        edn::value_t{ ' ' },
        testing::AllOf(
            testing::Property("if_character", &edn::value_t::if_character, testing::Pointee(' ')),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::character),
            SerializedIs(testing::StrEq("\\space"))));
}

TEST(edn, string)
{
    EXPECT_THAT(
        edn::value_t{ std::string{ "Benvenuto" } },
        testing::AllOf(
            testing::Property("if_string", &edn::value_t::if_string, testing::Pointee(testing::StrEq("Benvenuto"))),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::string),
            SerializedIs(testing::StrEq(R"("Benvenuto")"))));
}

TEST(edn, symbol)
{
    EXPECT_THAT(
        edn::value_t{ edn::symbol_t{ "my-symbol" } },
        testing::AllOf(
            testing::Property("if_symbol", &edn::value_t::if_symbol, testing::Pointee(testing::StrEq("my-symbol"))),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::symbol),
            SerializedIs(testing::StrEq("my-symbol"))));
}

TEST(edn, keyword)
{
    EXPECT_THAT(
        edn::value_t{ edn::keyword_t{ "my-keyword" } },
        testing::AllOf(
            testing::Property("if_keyword", &edn::value_t::if_keyword, testing::Pointee(testing::StrEq("my-keyword"))),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::keyword),
            SerializedIs(testing::StrEq(":my-keyword"))));
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
                            testing::Property("type", &edn::value_t::type, edn::value_type_t::string),
                            SerializedIs(testing::StrEq(R"("2024-01-01")"))))))),
            testing::Property("type", &edn::value_t::type, edn::value_type_t::tagged_element),
            SerializedIs(testing::StrEq(R"(#inst "2024-01-01")"))));
}
