#include <edn2.hpp>
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

void run(const std::vector<std::string>& args)
{
    edn2::value_t data
        = edn2::vector_t{ edn2::map_t{ { edn2::keyword_t{ "name" }, edn2::string_t{ "John" } },
                                       { edn2::keyword_t{ "age" }, 30 },
                                       { edn2::keyword_t{ "alive" }, true },
                                       { edn2::keyword_t{ "x" }, edn2::vector_t(5, 4) },
                                       { edn2::keyword_t{ "items" },
                                         edn2::vector_t{ edn2::string_t{ "apple" }, edn2::string_t{ "banana" } } } },
                          edn2::map_t{ { edn2::keyword_t{ "name" }, edn2::string_t{ "William" } },
                                       { edn2::keyword_t{ "age" }, 42 },
                                       { edn2::keyword_t{ "children" }, edn2::nil },
                                       { edn2::keyword_t{ "x" }, edn2::vector_t(5, 3) },
                                       { edn2::keyword_t{ "items" },
                                         edn2::vector_t{ edn2::string_t{ "watermelon" }, edn2::string_t{ "grape" } } } } };

    std::cout << data.type() << "\n";
    std::cout << data << "\n";

    edn2::pretty_print_options opts;
    opts.colors = edn2::color_scheme{};
    opts.indent_size = 2;
    opts.max_inline_length = 70;
    opts.compact_maps = false;
    edn2::pretty_print(std::cout, data, opts);
    edn2::pretty_print(std::cout, data.if_vector()->at(0).if_map()->at(edn2::keyword_t{ "x" }), opts);
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
