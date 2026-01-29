#include <edn.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct path_t : public std::string
{
    using base_t = std::string;

    explicit path_t(std::string_view path) : base_t(path)
    {
    }
};

inline auto load_file(std::istream& is) -> std::string
{
    return std::string(std::istreambuf_iterator<char>{ is }, std::istreambuf_iterator<char>{});
}

inline auto load_file(const path_t& path) -> std::string
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error{ std::string("cannot open '") + path + "'" };
    }
    return load_file(file);
}

void test_parse(const std::string& input)
{
    std::cout << "\n=== Parsing: " << input << " ===\n";
    try
    {
        edn::value_t result = edn::parse(input);
        std::cout << "Success! Result:\n";
        edn::pretty_print(std::cout, result);
    }
    catch (const edn::parse_error& e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
}

void demo_parser()
{
    std::cout << "EDN Parser with Location-Aware Error Reporting\n";
    std::cout << "===============================================\n";

    // Valid examples
    test_parse("42");
    test_parse("3.14");
    test_parse("true");
    test_parse("nil");
    test_parse(":keyword");
    test_parse("\"hello world\"");
    test_parse("\\newline");
    test_parse("symbol");
    test_parse("[1 2 3]");
    test_parse("(+ 1 2)");
    test_parse("{:name \"John\" :age 30}");
    test_parse("#{1 2 3}");
    test_parse("#inst \"2024-01-01\"");
    test_parse("'(1 2 3)");

    // Complex nested structure
    test_parse(R"(
    {:person {:name "Alice"
              :age 30
              :hobbies ["reading" "coding"]}
     :scores [95 87 92]}
    )");

    test_parse(R"(
    [{:person {:name "Alice"
              :age 30
              :hobbies ["reading" "coding"]}
     :scores [95 87 92]}
     {:person {:name "Alice"
              :age 30
              :hobbies ["reading" "coding"]}
     :scores [95 87 92]}]
    )");

    // Error examples - these will show line numbers
    test_parse("[1 2 3");                 // Unterminated vector
    test_parse("{:a 1 :b}");              // Odd number of map elements
    test_parse("\"unterminated string");  // Unterminated string

    // Multi-line error example
    test_parse(R"(
    [1 2 3
     4 5 6
     7 8
    )");  // Unterminated vector on line 4

    // Map error with location
    test_parse(R"(
    {:a 1
     :b 2
     :c 3
     :d}
    )");  // Odd number of elements - error on closing brace
}

void run(const std::vector<std::string>& args)
{
    // edn2::value_t data
    //     = edn2::vector_t{ edn2::map_t{ { edn2::keyword_t{ "name" }, edn2::string_t{ "John" } },
    //                                    { edn2::keyword_t{ "age" }, 30 },
    //                                    { edn2::keyword_t{ "alive" }, true },
    //                                    { edn2::keyword_t{ "x" }, edn2::vector_t(5, 4) },
    //                                    { edn2::keyword_t{ "items" },
    //                                      edn2::vector_t{ edn2::string_t{ "apple" }, edn2::string_t{ "banana" } } } },
    //                       edn2::map_t{ { edn2::keyword_t{ "name" }, edn2::string_t{ "William" } },
    //                                    { edn2::keyword_t{ "age" }, 42 },
    //                                    { edn2::keyword_t{ "children" }, edn2::nil },
    //                                    { edn2::keyword_t{ "x" }, edn2::vector_t(5, 3) },
    //                                    { edn2::keyword_t{ "items" },
    //                                      edn2::vector_t{ edn2::string_t{ "watermelon" }, edn2::string_t{ "grape" } } } }
    //                                      };

    // std::cout << data.type() << "\n";
    // std::cout << data << "\n";

    // edn2::pretty_print_options opts;
    // opts.colors = edn2::color_scheme{};
    // opts.indent_size = 2;
    // opts.max_inline_length = 70;
    // opts.compact_maps = false;
    // edn2::pretty_print(std::cout, data, opts);
    // edn2::pretty_print(std::cout, data.if_vector()->at(0).if_map()->at(edn2::keyword_t{ "x" }), opts);
    demo_parser();
}

int main(int argc, char** argv)
{
    try
    {
        run(std::vector<std::string>(argv, argv + argc));
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nError:\n" << ex.what() << "\n";
    }
}
