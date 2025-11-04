#include <catch2/catch_test_macros.hpp>
#include <edn.hpp>

TEST_CASE("edn", "[edn][parsing]")
{
    REQUIRE(edn::parse("3") == edn::integer_t{ 3 });
    REQUIRE(edn::parse("3.14") == edn::floating_point_t{ 3.14 });

    REQUIRE(edn::parse("nil") == edn::nil_t{});

    REQUIRE(edn::parse("true") == edn::boolean_t{ true });
    REQUIRE(edn::parse("false") == edn::boolean_t{ false });

    REQUIRE(edn::parse("\\space") == edn::character_t{ ' ' });
    REQUIRE(edn::parse("\\@") == edn::character_t{ '@' });

    REQUIRE(edn::parse("abc") == edn::symbol_t{ "abc" });
    REQUIRE(edn::parse(":abc") == edn::keyword_t{ "abc" });
    REQUIRE(edn::parse("\"abc\"") == edn::string_t{ "abc" });
    REQUIRE(edn::parse("\"abc def\"") == edn::string_t{ "abc def" });
}
