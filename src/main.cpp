#include <edn.hpp>
#include <iostream>

auto to_span(const edn::value_t& arg) -> edn::span<edn::value_t>
{
    return edn::span<edn::value_t>(&arg, 1);
}

auto print(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        arg.format(std::cout, edn::value_t::format_mode_t::str);
    }
    std::cout << std::endl;
    return {};
}

auto debug(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        arg.format(std::cout, edn::value_t::format_mode_t::repr);
    }
    std::cout << std::endl;
    return {};
}

auto odd_qm(edn::span<edn::value_t> args) -> edn::value_t
{
    if (const auto v = args.at(0).if_integer())
    {
        return *v % 2 != 0;
    }
    return {};
}

auto map(edn::span<edn::value_t> args) -> edn::value_t
{
    edn::value_t::list_t result;
    if (const auto callable = args.at(0).if_callable())
    {
        if (const auto v = args.at(1).if_vector())
        {
            for (const edn::value_t& item : *v)
            {
                result.push_back((*callable)(to_span(item)));
            }
        }
        if (const auto v = args.at(1).if_list())
        {
            for (const edn::value_t& item : *v)
            {
                result.push_back((*callable)(to_span(item)));
            }
        }
    }
    return result;
}

auto filter(edn::span<edn::value_t> args) -> edn::value_t
{
    edn::value_t::list_t result;
    if (const auto callable = args.at(0).if_callable())
    {
        if (const auto v = args.at(1).if_vector())
        {
            for (const edn::value_t& item : *v)
            {
                if (callable->test(to_span(item)))
                {
                    result.push_back(item);
                }
            }
        }
        if (const auto v = args.at(1).if_list())
        {
            for (const edn::value_t& item : *v)
            {
                if (callable->test(to_span(item)))
                {
                    result.push_back(item);
                }
            }
        }
    }
    return result;
}

template <class Op>
struct binary_op
{
    auto operator()(edn::span<edn::value_t> args) const -> edn::value_t
    {
        if (args.size() != 2)
        {
            throw std::runtime_error{ "binary_op: two arguments expected" };
        }
        static const auto op = Op{};
        {
            const auto lhs = args.at(0).if_integer();
            const auto rhs = args.at(1).if_integer();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        {
            const auto lhs = args.at(0).if_floating_point();
            const auto rhs = args.at(1).if_integer();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        {
            const auto lhs = args.at(0).if_integer();
            const auto rhs = args.at(1).if_floating_point();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        {
            const auto lhs = args.at(0).if_floating_point();
            const auto rhs = args.at(1).if_floating_point();
            if (lhs && rhs)
            {
                return op(*lhs, *rhs);
            }
        }

        return {};
    }
};

void run()
{
    auto stack = std::invoke(
        [&]() -> edn::stack_t
        {
            edn::stack_t result{ nullptr };
            result.insert(edn::value_t::symbol_t{ "print" }, edn::value_t::callable_t{ &print });
            result.insert(edn::value_t::symbol_t{ "debug" }, edn::value_t::callable_t{ &debug });

            result.insert(edn::value_t::symbol_t{ "+" }, edn::value_t::callable_t{ binary_op<std::plus<>>{} });
            result.insert(edn::value_t::symbol_t{ "-" }, edn::value_t::callable_t{ binary_op<std::minus<>>{} });
            result.insert(edn::value_t::symbol_t{ "*" }, edn::value_t::callable_t{ binary_op<std::multiplies<>>{} });
            result.insert(edn::value_t::symbol_t{ "/" }, edn::value_t::callable_t{ binary_op<std::divides<>>{} });

            result.insert(edn::value_t::symbol_t{ "=" }, edn::value_t::callable_t{ binary_op<std::equal_to<>>{} });
            result.insert(edn::value_t::symbol_t{ "!=" }, edn::value_t::callable_t{ binary_op<std::not_equal_to<>>{} });
            result.insert(edn::value_t::symbol_t{ "/=" }, edn::value_t::callable_t{ binary_op<std::not_equal_to<>>{} });
            result.insert(edn::value_t::symbol_t{ "<" }, edn::value_t::callable_t{ binary_op<std::less<>>{} });
            result.insert(edn::value_t::symbol_t{ ">" }, edn::value_t::callable_t{ binary_op<std::greater<>>{} });
            result.insert(edn::value_t::symbol_t{ "<=" }, edn::value_t::callable_t{ binary_op<std::less_equal<>>{} });
            result.insert(edn::value_t::symbol_t{ ">=" }, edn::value_t::callable_t{ binary_op<std::greater_equal<>>{} });

            result.insert(edn::value_t::symbol_t{ "odd?" }, edn::value_t::callable_t{ &odd_qm });
            result.insert(edn::value_t::symbol_t{ "map" }, edn::value_t::callable_t{ &map });
            result.insert(edn::value_t::symbol_t{ "filter" }, edn::value_t::callable_t{ &filter });
            return result;
        });

    const edn::value_t value = edn::parse(R"(
        (do
            (def v #{1 2 \space "text" #_ [\A \B ]})
            (debug v)
        )
    )");

    std::cout << value << "\n\n";
    const auto result = edn::evaluate(value, stack);
    std::cout << "> " << result << "\n";
}

int main()
{
    try
    {
        run();
    }
    catch (const std::exception& ex)
    {
        std::cout << "\n"
                  << "Error:"
                  << "\n"
                  << ex.what();
    }
}
