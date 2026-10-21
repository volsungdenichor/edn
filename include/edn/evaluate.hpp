#pragma once

#pragma once

#include <algorithm>
#include <edn/edn.hpp>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace edn
{

namespace evaluate_detail
{

enum class arithmetic_operator_t
{
    addition,
    subtraction,
    multiplication,
    division
};

struct arithmetic_t
{
    arithmetic_operator_t operation;
    std::string_view name;

    value_t operator()(const std::vector<value_t>& args) const
    {
        const bool has_identity
            = operation == arithmetic_operator_t::addition || operation == arithmetic_operator_t::multiplication;
        if (!has_identity && args.empty())
        {
            throw std::invalid_argument{ str("operator `", name, "` requires at least one argument") };
        }

        number_t result
            = operation == arithmetic_operator_t::multiplication ? number_t{ integer_t{ 1 } } : number_t{ integer_t{ 0 } };
        std::size_t index = 0;
        if (!has_identity)
        {
            result = as_number(args.front(), name);
            index = 1;
            if (args.size() == 1)
            {
                if (operation == arithmetic_operator_t::subtraction)
                {
                    result = apply_arithmetic(integer_t{ 0 }, result, operation, name);
                }
                else
                {
                    result = apply_arithmetic(integer_t{ 1 }, result, operation, name);
                }
            }
        }

        for (; index < args.size(); ++index)
        {
            result = apply_arithmetic(result, as_number(args.at(index), name), operation, name);
        }

        return std::visit([](auto value) -> value_t { return value; }, result);
    }

    static number_t apply_arithmetic(
        const number_t& lhs, const number_t& rhs, arithmetic_operator_t operation, std::string_view name)
    {
        if (const auto left = std::get_if<integer_t>(&lhs))
        {
            if (const auto right = std::get_if<integer_t>(&rhs))
            {
                switch (operation)
                {
                    case arithmetic_operator_t::addition:
                        if (const auto result = checked_add(*left, *right))
                        {
                            return *result;
                        }
                        break;
                    case arithmetic_operator_t::subtraction:
                        if (const auto result = checked_subtract(*left, *right))
                        {
                            return *result;
                        }
                        break;
                    case arithmetic_operator_t::multiplication:
                        if (const auto result = checked_multiply(*left, *right))
                        {
                            return *result;
                        }
                        break;
                    case arithmetic_operator_t::division:
                        if (*right == 0)
                        {
                            throw std::domain_error{ str("division by zero in `", name, "`") };
                        }
                        if (!(*left == std::numeric_limits<integer_t>::min() && *right == -1) && *left % *right == 0)
                        {
                            return *left / *right;
                        }
                        break;
                }
            }
        }

        const floating_point_t left = as_floating_point(lhs);
        const floating_point_t right = as_floating_point(rhs);
        switch (operation)
        {
            case arithmetic_operator_t::addition: return left + right;
            case arithmetic_operator_t::subtraction: return left - right;
            case arithmetic_operator_t::multiplication: return left * right;
            case arithmetic_operator_t::division:
                if (right == 0.0)
                {
                    throw std::domain_error{ str("division by zero in `", name, "`") };
                }
                return left / right;
        }
        throw std::logic_error{ "unknown arithmetic operator" };
    }

    static number_t as_number(const value_t& value, std::string_view name)
    {
        if (const auto integer = value.if_integer())
        {
            return *integer;
        }
        if (const auto floating_point = value.if_floating_point())
        {
            return *floating_point;
        }
        throw std::invalid_argument{ str("operator `", name, "` requires numeric arguments") };
    }

    static floating_point_t as_floating_point(const number_t& value)
    {
        if (const auto integer = std::get_if<integer_t>(&value))
        {
            return static_cast<floating_point_t>(*integer);
        }
        return std::get<floating_point_t>(value);
    }

    static std::optional<integer_t> checked_add(integer_t lhs, integer_t rhs)
    {
        constexpr auto min = std::numeric_limits<integer_t>::min();
        constexpr auto max = std::numeric_limits<integer_t>::max();
        if ((rhs > 0 && lhs > max - rhs) || (rhs < 0 && lhs < min - rhs))
        {
            return {};
        }
        return lhs + rhs;
    }

    static std::optional<integer_t> checked_subtract(integer_t lhs, integer_t rhs)
    {
        constexpr auto min = std::numeric_limits<integer_t>::min();
        constexpr auto max = std::numeric_limits<integer_t>::max();
        if ((rhs < 0 && lhs > max + rhs) || (rhs > 0 && lhs < min + rhs))
        {
            return {};
        }
        return lhs - rhs;
    }

    static std::optional<integer_t> checked_multiply(integer_t lhs, integer_t rhs)
    {
        constexpr auto min = std::numeric_limits<integer_t>::min();
        constexpr auto max = std::numeric_limits<integer_t>::max();
        if (lhs == 0 || rhs == 0)
        {
            return 0;
        }
        if ((lhs == -1 && rhs == min) || (rhs == -1 && lhs == min))
        {
            return {};
        }
        if ((lhs > 0 && rhs > 0 && lhs > max / rhs) || (lhs > 0 && rhs < 0 && rhs < min / lhs)
            || (lhs < 0 && rhs > 0 && lhs < min / rhs) || (lhs < 0 && rhs < 0 && lhs < max / rhs))
        {
            return {};
        }
        return lhs * rhs;
    }
};

inline bool is_truthy(const value_t& value)
{
    const auto boolean = value.if_boolean();
    return value.type() != value_type_t::nil && (!boolean || *boolean);
}

enum class comparison_operator_t
{
    equal,
    not_equal,
    less,
    less_equal,
    greater,
    greater_equal
};

struct comparison_t
{
    comparison_operator_t operation;
    std::string_view name;

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.empty())
        {
            throw std::invalid_argument{ str("operator `", name, "` requires at least one argument") };
        }
        if (operation == comparison_operator_t::equal || operation == comparison_operator_t::not_equal)
        {
            const bool equal = std::all_of(
                args.begin() + 1, args.end(), [&](const value_t& value) { return args.front() == value; });
            return operation == comparison_operator_t::equal ? equal : !equal;
        }

        auto previous = arithmetic_t::as_floating_point(arithmetic_t::as_number(args.front(), name));
        bool result = true;
        for (auto it = args.begin() + 1; it != args.end(); ++it)
        {
            const auto current = arithmetic_t::as_floating_point(arithmetic_t::as_number(*it, name));
            switch (operation)
            {
                case comparison_operator_t::less: result = (previous < current) && result; break;
                case comparison_operator_t::less_equal: result = (previous <= current) && result; break;
                case comparison_operator_t::greater: result = (previous > current) && result; break;
                case comparison_operator_t::greater_equal: result = (previous >= current) && result; break;
                default: throw std::logic_error{ "unknown numeric comparison operator" };
            }
            previous = current;
        }
        return result;
    }
};

struct predicate_t
{
    std::string_view name;
    bool (*test)(const value_t&);

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.size() != 1)
        {
            throw std::invalid_argument{ str("`", name, "` requires exactly one argument") };
        }
        return test(args.front());
    }
};

static const callable_t& as_callable(const value_t& value, std::string_view name)
{
    if (const auto callable = value.if_callable())
    {
        return *callable;
    }
    throw std::invalid_argument{ str("`", name, "` requires callable arguments, got ", value) };
}

struct partial_t
{
    struct impl_t
    {
        value_t function;
        std::vector<value_t> bound_args;

        value_t operator()(const std::vector<value_t>& args) const
        {
            std::vector<value_t> all_args;
            all_args.reserve(bound_args.size() + args.size());
            all_args.insert(all_args.end(), bound_args.begin(), bound_args.end());
            all_args.insert(all_args.end(), args.begin(), args.end());
            return as_callable(function, "partial")(all_args);
        }
    };

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.empty())
        {
            throw std::invalid_argument{ "`partial` requires at least one argument" };
        }
        as_callable(args.front(), "partial");
        if (args.size() == 1)
        {
            return args.front();
        }
        return callable_t{ impl_t{ args.front(), std::vector<value_t>(args.begin() + 1, args.end()) } };
    }
};

struct comp_t
{
    struct impl_t
    {
        std::vector<value_t> functions;

        value_t operator()(const std::vector<value_t>& args) const
        {
            auto it = functions.rbegin();
            value_t result = as_callable(*it, "comp")(args);
            for (++it; it != functions.rend(); ++it)
            {
                result = as_callable(*it, "comp")(std::vector<value_t>{ result });
            }
            return result;
        }
    };

    value_t operator()(const std::vector<value_t>& args) const
    {
        if (args.empty())
        {
            return callable_t{ [](const std::vector<value_t>& inner) -> value_t
                               {
                                   if (inner.size() != 1)
                                   {
                                       throw std::invalid_argument{
                                           "identity function returned by `(comp)` requires exactly one argument"
                                       };
                                   }
                                   return inner.front();
                               } };
        }
        for (const value_t& arg : args)
        {
            as_callable(arg, "comp");
        }
        if (args.size() == 1)
        {
            return args.front();
        }
        return callable_t{ impl_t{ args } };
    }
};

}  // namespace evaluate_detail

namespace evaluate_detail
{

using frame_type = std::map<symbol_t, value_t>;

struct scope_t
{
    std::shared_ptr<const scope_t> outer;
    std::weak_ptr<scope_t> root;
    frame_type frame;
    bool is_root = false;
};

}  // namespace evaluate_detail

class stack_t
{
public:
    using frame_type = evaluate_detail::frame_type;

    stack_t() : stack_t{ frame_type{}, nullptr } { }

    stack_t(frame_type frame, const stack_t* outer) : m_scope{ std::make_shared<evaluate_detail::scope_t>() }
    {
        m_scope->frame = std::move(frame);
        m_scope->is_root = outer == nullptr;
        if (outer)
        {
            m_scope->outer = outer->locals();
            m_scope->root = outer->root();
        }
    }

    stack_t(const stack_t* outer) : stack_t{ frame_type{}, outer } { }

    const value_t& insert(const symbol_t& symbol, const value_t& v)
    {
        m_scope->frame.emplace(symbol, v);
        return v;
    }

    const value_t& define(const symbol_t& symbol, const value_t& v)
    {
        const auto root = lock_root();
        root->frame.emplace(symbol, v);
        return v;
    }

    const value_t& get(const symbol_t& symbol) const
    {
        for (const evaluate_detail::scope_t* scope = locals().get(); scope; scope = scope->outer.get())
        {
            if (const auto iter = scope->frame.find(symbol); iter != scope->frame.end())
            {
                return iter->second;
            }
        }

        const auto root = lock_root();
        if (const auto iter = root->frame.find(symbol); iter != root->frame.end())
        {
            return iter->second;
        }
        throw std::runtime_error{ str("Unrecognized symbol `", symbol, "`") };
    }

    const value_t& operator[](const symbol_t& symbol) const { return get(symbol); }

private:
    friend struct evaluate_fn;

    struct captured_t
    {
        std::shared_ptr<const evaluate_detail::scope_t> locals;
        std::weak_ptr<evaluate_detail::scope_t> root;
    };

    stack_t(frame_type frame, const captured_t& captured) : m_scope{ std::make_shared<evaluate_detail::scope_t>() }
    {
        m_scope->frame = std::move(frame);
        m_scope->outer = captured.locals;
        m_scope->root = captured.root;
    }

    auto locals() const -> std::shared_ptr<const evaluate_detail::scope_t> { return m_scope->is_root ? nullptr : m_scope; }

    auto root() const -> std::weak_ptr<evaluate_detail::scope_t> { return m_scope->is_root ? m_scope : m_scope->root; }

    auto capture() const -> captured_t { return { locals(), root() }; }

    auto lock_root() const -> std::shared_ptr<evaluate_detail::scope_t>
    {
        if (auto root_scope = root().lock())
        {
            return root_scope;
        }
        throw std::logic_error{ "the root environment of this scope has been destroyed" };
    }

    std::shared_ptr<evaluate_detail::scope_t> m_scope;
};

inline auto make_standard_environment() -> stack_t
{
    stack_t environment;
    environment.insert(
        symbol_t{ "+" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::addition, "+" } });
    environment.insert(
        symbol_t{ "-" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::subtraction, "-" } });
    environment.insert(
        symbol_t{ "*" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::multiplication, "*" } });
    environment.insert(
        symbol_t{ "/" },
        callable_t{ evaluate_detail::arithmetic_t{ evaluate_detail::arithmetic_operator_t::division, "/" } });
    environment.insert(symbol_t{ "partial" }, callable_t{ evaluate_detail::partial_t{} });
    environment.insert(symbol_t{ "comp" }, callable_t{ evaluate_detail::comp_t{} });
    using comparison = evaluate_detail::comparison_operator_t;
    for (const auto& [name, operation] : {
             std::pair{ "=", comparison::equal },
             std::pair{ "not=", comparison::not_equal },
             std::pair{ "<", comparison::less },
             std::pair{ "<=", comparison::less_equal },
             std::pair{ ">", comparison::greater },
             std::pair{ ">=", comparison::greater_equal } })
    {
        environment.insert(symbol_t{ name }, callable_t{ evaluate_detail::comparison_t{ operation, name } });
    }
    for (const auto& predicate : {
             evaluate_detail::predicate_t{ "nil?", [](const value_t& v) { return v.type() == value_type_t::nil; } },
             evaluate_detail::predicate_t{ "boolean?", [](const value_t& v) { return v.if_boolean() != nullptr; } },
             evaluate_detail::predicate_t{ "true?", [](const value_t& v) { return v.if_boolean() && *v.if_boolean(); } },
             evaluate_detail::predicate_t{ "false?", [](const value_t& v) { return v.if_boolean() && !*v.if_boolean(); } },
             evaluate_detail::predicate_t{ "number?", [](const value_t& v) { return v.if_integer() || v.if_floating_point(); } },
             evaluate_detail::predicate_t{ "integer?", [](const value_t& v) { return v.if_integer() != nullptr; } },
             evaluate_detail::predicate_t{ "float?", [](const value_t& v) { return v.if_floating_point() != nullptr; } },
             evaluate_detail::predicate_t{ "char?", [](const value_t& v) { return v.if_character() != nullptr; } },
             evaluate_detail::predicate_t{ "string?", [](const value_t& v) { return v.if_string() != nullptr; } },
             evaluate_detail::predicate_t{ "symbol?", [](const value_t& v) { return v.if_symbol() != nullptr; } },
             evaluate_detail::predicate_t{ "keyword?", [](const value_t& v) { return v.if_keyword() != nullptr; } },
             evaluate_detail::predicate_t{ "list?", [](const value_t& v) { return v.if_list() != nullptr; } },
             evaluate_detail::predicate_t{ "vector?", [](const value_t& v) { return v.if_vector() != nullptr; } },
             evaluate_detail::predicate_t{ "set?", [](const value_t& v) { return v.if_set() != nullptr; } },
             evaluate_detail::predicate_t{ "map?", [](const value_t& v) { return v.if_map() != nullptr; } },
             evaluate_detail::predicate_t{ "fn?", [](const value_t& v) { return v.if_callable() != nullptr; } },
             evaluate_detail::predicate_t{ "coll?", [](const value_t& v) {
                 return v.if_list() || v.if_vector() || v.if_set() || v.if_map();
             } },
             evaluate_detail::predicate_t{ "sequential?", [](const value_t& v) { return v.if_list() || v.if_vector(); } },
             evaluate_detail::predicate_t{ "not", [](const value_t& v) { return !evaluate_detail::is_truthy(v); } } })
    {
        environment.insert(symbol_t{ predicate.name }, callable_t{ predicate });
    }
    return environment;
}

constexpr inline struct evaluate_fn
{
    auto operator()(const value_t& value, stack_t& stack) const -> value_t { return eval(value, stack); }

private:
    auto eval(const value_t& value, stack_t& stack) const -> value_t
    {
        try
        {
            return do_eval(value, stack);
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error{ str("Error on evaluating `", value, "`: ", ex.what()) };
        }
    }

    auto do_eval(const value_t& value, stack_t& stack) const -> value_t
    {
        if (auto maybe_quoted_element = value.if_quoted_element())
        {
            return maybe_quoted_element->element();
        }
        else if (auto maybe_symbol = value.if_symbol())
        {
            return stack[*maybe_symbol];
        }
        else if (auto maybe_list = value.if_list())
        {
            return eval_list(*maybe_list, stack);
        }
        else if (auto maybe_vector = value.if_vector())
        {
            vector_t res = {};
            res.reserve(maybe_vector->size());
            for (const value_t& item : *maybe_vector)
            {
                res.push_back(do_eval(item, stack));
            }
            return res;
        }
        else if (auto maybe_set = value.if_set())
        {
            set_t res = {};
            for (const value_t& item : *maybe_set)
            {
                res.insert(do_eval(item, stack));
            }
            return res;
        }
        else if (auto maybe_map = value.if_map())
        {
            map_t res = {};
            for (const auto& [key, val] : *maybe_map)
            {
                res.emplace(do_eval(key, stack), do_eval(val, stack));
            }
            return res;
        }
        return value;
    }

    struct clojure_t
    {
        struct overload_t
        {
            value_t parameters;
            std::vector<value_t> body;

            auto params() const -> std::tuple<std::vector<symbol_t>, std::optional<symbol_t>>
            {
                std::vector<symbol_t> mandatory;
                std::vector<symbol_t> variadic;
                std::vector<symbol_t>* current = &mandatory;
                for (const value_t& v : *parameters.if_vector())
                {
                    const symbol_t& s = *v.if_symbol();
                    if (s == symbol_t{ "&" })
                    {
                        current = &variadic;
                    }
                    else
                    {
                        current->push_back(s);
                    }
                }
                return { std::move(mandatory),
                         !variadic.empty()  //
                             ? std::optional<symbol_t>{ variadic.at(0) }
                             : std::optional<symbol_t>{} };
            }
        };

        const evaluate_fn& self;
        std::vector<overload_t> overloads;
        stack_t::captured_t captured;

        auto operator()(const std::vector<value_t>& args) -> value_t
        {
            for (const overload_t& overload : overloads)
            {
                const auto [mandatory, maybe_variadic] = overload.params();

                if (args.size() == mandatory.size() && !maybe_variadic)
                {
                    stack_t::frame_type frame;
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        frame.emplace(mandatory.at(i), args.at(i));
                    }
                    stack_t new_stack{ std::move(frame), captured };
                    return self.eval_block(overload.body, new_stack);
                }
                if (args.size() > mandatory.size() && maybe_variadic)
                {
                    stack_t::frame_type frame;
                    list_t tail;
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        if (i < mandatory.size())
                        {
                            frame.emplace(mandatory.at(i), args.at(i));
                        }
                        else
                        {
                            tail.push_back(args.at(i));
                        }
                    }
                    frame.emplace(*maybe_variadic, std::move(tail));
                    stack_t new_stack{ std::move(frame), captured };
                    return self.eval_block(overload.body, new_stack);
                }
            }
            throw std::runtime_error{ str("could not resolve function overload for ", args.size(), " arg(s)") };
        };
    };

    auto eval_block(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return do_eval(item, stack); });
    }

    auto eval_let(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        const auto& bindings = *input.at(0).if_vector();
        stack_t current = stack;
        for (std::size_t i = 0; i < bindings.size(); i += 2)
        {
            value_t value = do_eval(bindings.at(i + 1), current);
            current = stack_t{ stack_t::frame_type{ { *bindings.at(i + 0).if_symbol(), std::move(value) } }, &current };
        }
        return eval_block({ input.begin() + 1, input.end() }, current);
    }

    auto eval_def(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return stack.define(*input.at(0).if_symbol(), do_eval(input.at(1), stack));
    }

    static auto create_overload(const std::vector<value_t>& input) -> clojure_t::overload_t
    {
        value_t parameters = *input.at(0).if_vector();
        return clojure_t::overload_t{ std::move(parameters), { input.begin() + 1, input.end() } };
    }

    auto eval_callable(const std::vector<value_t>& input, stack_t& stack) const -> callable_t
    {
        std::vector<clojure_t::overload_t> overloads;
        if (std::all_of(input.begin(), input.end(), [](const value_t& v) { return v.if_list(); }))
        {
            for (const value_t& v : input)
            {
                overloads.push_back(create_overload(*v.if_list()));
            }
        }
        else
        {
            overloads.push_back(create_overload(input));
        }
        return callable_t{ clojure_t{ *this, std::move(overloads), stack.capture() } };
    }

    auto eval_fn(const std::vector<value_t>& input, stack_t& stack) const -> value_t { return eval_callable(input, stack); }

    auto eval_defn(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return stack.define(*input.at(0).if_symbol(), eval_callable({ input.begin() + 1, input.end() }, stack));
    }

    auto eval_boolean(const value_t& value, stack_t& stack) const -> bool
    {
        return evaluate_detail::is_truthy(do_eval(value, stack));
    }

    auto eval_and(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        value_t result = true;
        for (const value_t& item : input)
        {
            result = do_eval(item, stack);
            if (!evaluate_detail::is_truthy(result))
            {
                break;
            }
        }
        return result;
    }

    auto eval_or(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        value_t result = nil;
        for (const value_t& item : input)
        {
            result = do_eval(item, stack);
            if (evaluate_detail::is_truthy(result))
            {
                break;
            }
        }
        return result;
    }

    auto eval_if(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return eval_boolean(input.at(0), stack) ? do_eval(input.at(1), stack) : do_eval(input.at(2), stack);
    }

    auto eval_cond(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        for (std::size_t i = 0; i < input.size(); i += 2)
        {
            if (input.at(i + 0) == keyword_t{ "else" } || eval_boolean(input.at(i + 0), stack))
            {
                return do_eval(input.at(i + 1), stack);
            }
        }
        return value_t{};
    }

    auto eval_callable(const value_t& head, const std::vector<value_t>& tail, stack_t& stack) const -> value_t
    {
        const callable_t callable = evaluate_detail::as_callable(do_eval(head, stack), "callable");
        std::vector<value_t> args;
        args.reserve(tail.size());
        for (const value_t& item : tail)
        {
            args.push_back(do_eval(item, stack));
        }
        return callable(args);
    }

    auto eval_quote(const std::vector<value_t>& input, stack_t&) const -> value_t { return input[0]; }

    auto eval_list(const list_t& input, stack_t& stack) const -> value_t
    {
        if (input.empty())
        {
            return nil;
        }
        const value_t& head = input.at(0);
        const auto tail = std::vector<value_t>(input.begin() + 1, input.end());

        using handler_t = value_t (evaluate_fn::*)(const std::vector<value_t>&, stack_t&) const;

        static const std::map<symbol_t, handler_t> handlers = {
            { symbol_t{ "quote" }, &evaluate_fn::eval_quote },  //
            { symbol_t{ "let" }, &evaluate_fn::eval_let },      //
            { symbol_t{ "def" }, &evaluate_fn::eval_def },      //
            { symbol_t{ "fn" }, &evaluate_fn::eval_fn },        //
            { symbol_t{ "defn" }, &evaluate_fn::eval_defn },    //
            { symbol_t{ "if" }, &evaluate_fn::eval_if },        //
            { symbol_t{ "cond" }, &evaluate_fn::eval_cond },    //
            { symbol_t{ "do" }, &evaluate_fn::eval_do },        //
            { symbol_t{ "and" }, &evaluate_fn::eval_and },      //
            { symbol_t{ "or" }, &evaluate_fn::eval_or },        //
        };

        if (const auto h = head.if_symbol())
        {
            if (const auto handler = handlers.find(*h); handler != handlers.end())
            {
                return std::invoke(handler->second, this, tail, stack);
            }
        }
        return eval_callable(head, tail, stack);
    }

    auto eval_do(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return std::accumulate(
            input.begin(),
            input.end(),
            value_t{},
            [&](const value_t&, const value_t& item) -> value_t { return do_eval(item, stack); });
    }
} evaluate{};

}  // namespace edn
