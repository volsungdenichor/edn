#include <edn.hpp>
#include <iostream>
#include <memory>

auto to_span(const edn::value_t& arg) -> edn::span<edn::value_t>
{
    return edn::span<edn::value_t>(&arg, 1);
}

auto type(edn::span<edn::value_t> args) -> edn::value_t
{
    switch (args.at(0).type())
    {
#define CASE(x) \
    case edn::type_t::x: return edn::keyword_t{ #x }
        CASE(nil);
        CASE(boolean);
        CASE(integer);
        CASE(floating_point);
        CASE(string);
        CASE(character);
        CASE(symbol);
        CASE(keyword);
        CASE(tagged_element);
        CASE(list);
        CASE(vector);
        CASE(set);
        CASE(map);
        CASE(callable);
#undef CASE
        default: return edn::keyword_t{ "nil" };
    }
}

auto print(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        arg.format(std::cout, edn::format_mode_t::str);
    }
    std::cout << std::endl;
    return {};
}

auto debug(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        arg.format(std::cout, edn::format_mode_t::repr);
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
    edn::list_t result;
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
    edn::list_t result;
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
            result.insert(edn::symbol_t{ "type" }, edn::callable_t{ &type });
            result.insert(edn::symbol_t{ "print" }, edn::callable_t{ &print });
            result.insert(edn::symbol_t{ "println" }, edn::callable_t{ &print });
            result.insert(edn::symbol_t{ "debug" }, edn::callable_t{ &debug });

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

            result.insert(edn::symbol_t{ "odd?" }, edn::callable_t{ &odd_qm });
            result.insert(edn::symbol_t{ "map" }, edn::callable_t{ &map });
            result.insert(edn::symbol_t{ "filter" }, edn::callable_t{ &filter });
            return result;
        });

    const edn::value_t value = edn::parse(R"(

        (println '(all [(field "first-name" (eq "Adam")) (field "last-name" (eq "Mickiewicz") ) ]))

        (defn get_info [x] {:value x :type (type x)})
        (print (get_info true))
        (print (get_info false))
        (print (get_info nil))
        (print (get_info 53))
        (print (get_info 2.71))
        (print (get_info \X))
        (print (get_info "ABC"))
        (print (get_info :abc))
        (print (get_info 'abc))
        (print (get_info '(:A :B :C)))
        (print (get_info #{:A :B :C}))
        (print (get_info {:name "Gumball"}))
        (print (get_info [:A :B :C]))

        (print ">>> " ''(+ 2 3))
    )");

    std::cout << "expr: " << value << "\n\n";
    const auto result = edn::evaluate(value, stack);
    std::cout << "result: " << result << "\n";
}

int main()
{
    try
    {
        run();
    }
    catch (const std::exception& ex)
    {
        std::cout << "\nError:\n" << ex.what() << "\n";
    }
}
