#include <edn.hpp>
#include <iostream>
#include <memory>

auto to_span(const edn::value_t& arg) -> edn::span<edn::value_t>
{
    return edn::span<edn::value_t>(&arg, 1);
}

auto type(edn::span<edn::value_t> args) -> edn::value_t
{
    static const std::map<edn::type_t, edn::keyword_t> map = {
        { edn::type_t::nil, edn::keyword_t(edn::str(edn::type_t::nil).c_str()) },
        { edn::type_t::boolean, edn::keyword_t(edn::str(edn::type_t::boolean).c_str()) },
        { edn::type_t::integer, edn::keyword_t(edn::str(edn::type_t::integer).c_str()) },
        { edn::type_t::floating_point, edn::keyword_t(edn::str(edn::type_t::floating_point).c_str()) },
        { edn::type_t::string, edn::keyword_t(edn::str(edn::type_t::string).c_str()) },
        { edn::type_t::character, edn::keyword_t(edn::str(edn::type_t::character).c_str()) },
        { edn::type_t::symbol, edn::keyword_t(edn::str(edn::type_t::symbol).c_str()) },
        { edn::type_t::keyword, edn::keyword_t(edn::str(edn::type_t::keyword).c_str()) },
        { edn::type_t::tagged_element, edn::keyword_t(edn::str(edn::type_t::tagged_element).c_str()) },
        { edn::type_t::list, edn::keyword_t(edn::str(edn::type_t::list).c_str()) },
        { edn::type_t::vector, edn::keyword_t(edn::str(edn::type_t::vector).c_str()) },
        { edn::type_t::set, edn::keyword_t(edn::str(edn::type_t::set).c_str()) },
        { edn::type_t::map, edn::keyword_t(edn::str(edn::type_t::map).c_str()) },
        { edn::type_t::callable, edn::keyword_t(edn::str(edn::type_t::callable).c_str()) },
    };
    return map.at(edn::type(args.at(0)));
}

auto print(edn::span<edn::value_t> args) -> edn::value_t
{
    for (const edn::value_t& arg : args)
    {
        std::cout << arg;
    }
    std::cout << std::endl;
    return {};
}

auto odd_qm(edn::span<edn::value_t> args) -> edn::value_t
{
    if (const auto v = std::get_if<edn::integer_t>(&args.at(0)))
    {
        return *v % 2 != 0;
    }
    return {};
}

auto map(edn::span<edn::value_t> args) -> edn::value_t
{
    edn::list_t result;
    if (const auto callable = std::get_if<edn::callable_t>(&args.at(0)))
    {
        if (const auto v = std::get_if<edn::vector_t>(&args.at(1)))
        {
            for (const edn::value_t& item : *v)
            {
                result.push_back((*callable)(to_span(item)));
            }
        }
        if (const auto v = std::get_if<edn::list_t>(&args.at(1)))
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
    if (const auto callable = std::get_if<edn::callable_t>(&args.at(0)))
    {
        if (const auto v = std::get_if<edn::vector_t>(&args.at(1)))
        {
            for (const edn::value_t& item : *v)
            {
                if (callable->test(to_span(item)))
                {
                    result.push_back(item);
                }
            }
        }
        if (const auto v = std::get_if<edn::list_t>(&args.at(1)))
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
        return std::visit(
            edn::overloaded{ [](edn::integer_t lt, edn::integer_t rt) -> edn::value_t { return op(lt, rt); },
                             [](edn::floating_point_t lt, edn::integer_t rt) -> edn::value_t { return op(lt, rt); },
                             [](edn::integer_t lt, edn::floating_point_t rt) -> edn::value_t { return op(lt, rt); },
                             [](edn::floating_point_t lt, edn::floating_point_t rt) -> edn::value_t { return op(lt, rt); },
                             [](const auto&, const auto&) -> edn::value_t { return edn::nil_t{}; }

            },
            args.at(0),
            args.at(1));
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
            result.insert(edn::symbol_t{ "debug" }, edn::callable_t{ &print });

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

        (println '(and [(field "first-name" (eq "Adam")) (field "last-name" (eq "Mickiewicz") ) ]))

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
        (+ 53 (* 2 3))
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
