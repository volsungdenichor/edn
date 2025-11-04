#include <edn.hpp>
#include <fstream>
#include <iostream>
#include <memory>

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
        throw std::runtime_error{ edn::str("cannot open '", path, '"') };
    }
    return load_file(file);
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <class Op>
struct binary_op
{
    auto operator()(const std::vector<edn::value_t>& args) const -> edn::value_t
    {
        if (args.size() != 2)
        {
            throw std::runtime_error{ "binary_op: two arguments expected" };
        }
        static const auto op = Op{};
        return std::visit(
            overloaded{ [](edn::integer_t lt, edn::integer_t rt) -> edn::value_t { return op(lt, rt); },
                        [](edn::floating_point_t lt, edn::integer_t rt) -> edn::value_t { return op(lt, rt); },
                        [](edn::integer_t lt, edn::floating_point_t rt) -> edn::value_t { return op(lt, rt); },
                        [](edn::floating_point_t lt, edn::floating_point_t rt) -> edn::value_t { return op(lt, rt); },
                        [](const auto&, const auto&) -> edn::value_t { return edn::nil_t{}; }

            },
            args.at(0).m_data,
            args.at(1).m_data);
    }
};

auto type(const std::vector<edn::value_t>& args) -> edn::value_t
{
    static const auto name = [](edn::value_type_t t) -> edn::symbol_t { return edn::symbol_t{ edn::str(t).c_str() }; };
    return name(args.at(0).type());
}

auto print(const std::vector<edn::value_t>& args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        if (auto v = arg.string())
        {
            std::cout << *v;
        }
        else
        {
            std::cout << arg;
        }
    }
    std::cout << std::endl;
    return {};
}

void run(const std::vector<std::string>& args)
{
    auto stack = std::invoke(
        [&]() -> edn::stack_t
        {
            edn::stack_t result{ nullptr };
            result.insert(edn::symbol_t{ "type" }, edn::callable_t{ &type });
            result.insert(edn::symbol_t{ "print" }, edn::callable_t{ &print });
            result.insert(edn::symbol_t{ "+" }, edn::callable_t{ binary_op<std::plus<>>{} });
            result.insert(edn::symbol_t{ "-" }, edn::callable_t{ binary_op<std::minus<>>{} });
            result.insert(edn::symbol_t{ "*" }, edn::callable_t{ binary_op<std::multiplies<>>{} });
            result.insert(edn::symbol_t{ "/" }, edn::callable_t{ binary_op<std::divides<>>{} });

            result.insert(edn::symbol_t{ "=" }, edn::callable_t{ binary_op<std::equal_to<>>{} });
            result.insert(edn::symbol_t{ "!=" }, edn::callable_t{ binary_op<std::not_equal_to<>>{} });
            result.insert(edn::symbol_t{ "/=" }, edn::callable_t{ binary_op<std::not_equal_to<>>{} });
            result.insert(edn::symbol_t{ "<" }, edn::callable_t{ binary_op<std::less<>>{} });
            result.insert(edn::symbol_t{ ">" }, edn::callable_t{ binary_op<std::greater<>>{} });
            result.insert(edn::symbol_t{ "<=" }, edn::callable_t{ binary_op<std::less_equal<>>{} });
            result.insert(edn::symbol_t{ ">=" }, edn::callable_t{ binary_op<std::greater_equal<>>{} });

            return result;
        });
    const path_t path = args.size() >= 2 ? path_t{ args.at(1) } : path_t{ "../src/program.txt" };

    const std::string file_content = load_file(path);

    const edn::value_t value = edn::parse(file_content);
    std::cout << "expr: " << value << "\n\n";
    const edn::value_t result = edn::evaluate(value, stack);
    std::cout << "result: " << result << "\n";
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
