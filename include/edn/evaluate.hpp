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
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

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

using number_t = std::variant<integer_t, floating_point_t>;

inline auto as_number(const value_t& value, std::string_view name) -> number_t
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

inline auto as_floating_point(const number_t& value) -> floating_point_t
{
    if (const auto integer = std::get_if<integer_t>(&value))
    {
        return static_cast<floating_point_t>(*integer);
    }
    return std::get<floating_point_t>(value);
}

inline auto checked_add(integer_t lhs, integer_t rhs) -> std::optional<integer_t>
{
    constexpr auto min = std::numeric_limits<integer_t>::min();
    constexpr auto max = std::numeric_limits<integer_t>::max();
    if ((rhs > 0 && lhs > max - rhs) || (rhs < 0 && lhs < min - rhs))
    {
        return {};
    }
    return lhs + rhs;
}

inline auto checked_subtract(integer_t lhs, integer_t rhs) -> std::optional<integer_t>
{
    constexpr auto min = std::numeric_limits<integer_t>::min();
    constexpr auto max = std::numeric_limits<integer_t>::max();
    if ((rhs < 0 && lhs > max + rhs) || (rhs > 0 && lhs < min + rhs))
    {
        return {};
    }
    return lhs - rhs;
}

inline auto checked_multiply(integer_t lhs, integer_t rhs) -> std::optional<integer_t>
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
    if ((lhs > 0 && rhs > 0 && lhs > max / rhs) ||
        (lhs > 0 && rhs < 0 && rhs < min / lhs) ||
        (lhs < 0 && rhs > 0 && lhs < min / rhs) ||
        (lhs < 0 && rhs < 0 && lhs < max / rhs))
    {
        return {};
    }
    return lhs * rhs;
}

inline auto apply_arithmetic(
    const number_t& lhs,
    const number_t& rhs,
    arithmetic_operator_t operation,
    std::string_view name) -> number_t
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
                    if (!(*left == std::numeric_limits<integer_t>::min() && *right == -1) &&
                        *left % *right == 0)
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

inline auto arithmetic_callable(arithmetic_operator_t operation, std::string_view name) -> value_t
{
    return callable_t{
        [operation, name = std::string{ name }](const std::vector<value_t>& args) -> value_t
        {
            const bool has_identity = operation == arithmetic_operator_t::addition ||
                                      operation == arithmetic_operator_t::multiplication;
            if (!has_identity && args.empty())
            {
                throw std::invalid_argument{ str("operator `", name, "` requires at least one argument") };
            }

            number_t result = operation == arithmetic_operator_t::multiplication ? number_t{ integer_t{ 1 } }
                                                                                 : number_t{ integer_t{ 0 } };
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
        }};
}

inline auto as_callable(const value_t& value, std::string_view name) -> const callable_t&
{
    if (const auto callable = value.if_callable())
    {
        return *callable;
    }
    throw std::invalid_argument{ str("`", name, "` requires callable arguments, got ", value) };
}

// `partial` and `comp` are named types (not lambdas) so the scope collector can see the
// callables they hold and trace reference cycles through them.
struct partial_t
{
    value_t function;
    std::vector<value_t> bound_args;

    auto operator()(const std::vector<value_t>& args) const -> value_t
    {
        std::vector<value_t> all_args;
        all_args.reserve(bound_args.size() + args.size());
        all_args.insert(all_args.end(), bound_args.begin(), bound_args.end());
        all_args.insert(all_args.end(), args.begin(), args.end());
        return (*function.if_callable())(all_args);
    }
};

struct comp_t
{
    // Applied right to left.
    std::vector<value_t> functions;

    auto operator()(const std::vector<value_t>& args) const -> value_t
    {
        auto it = functions.rbegin();
        value_t result = (*it->if_callable())(args);
        for (++it; it != functions.rend(); ++it)
        {
            result = (*it->if_callable())(std::vector<value_t>{ result });
        }
        return result;
    }
};

inline auto partial_callable() -> value_t
{
    return callable_t{ [](const std::vector<value_t>& args) -> value_t
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
                           return callable_t{ partial_t{ args.front(), std::vector<value_t>(args.begin() + 1, args.end()) } };
                       } };
}

inline auto comp_callable() -> value_t
{
    return callable_t{ [](const std::vector<value_t>& args) -> value_t
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
                           return callable_t{ comp_t{ args } };
                       } };
}

}  // namespace evaluate_detail

namespace evaluate_detail
{

using frame_type = std::map<symbol_t, value_t>;

class scope_registry_t;

struct scope_t : public std::enable_shared_from_this<scope_t>
{
    // Declared first so that it is destroyed last.
    std::shared_ptr<scope_registry_t> registry;
    std::shared_ptr<scope_t> outer;
    frame_type frame;

    scope_t(std::shared_ptr<scope_registry_t> r, std::shared_ptr<scope_t> o, frame_type f)
        : registry{ std::move(r) }
        , outer{ std::move(o) }
        , frame{ std::move(f) }
    {
    }

    scope_t(const scope_t&) = delete;
    scope_t& operator=(const scope_t&) = delete;

    ~scope_t();
};

// Tracks every scope created through one stack_manager_t and frees scopes that are only
// kept alive by reference cycles (e.g. a function stored in the scope it closes over).
class scope_registry_t
{
public:
    explicit scope_registry_t(std::size_t min_threshold)
        : m_min_threshold{ std::max<std::size_t>(min_threshold, 1) }
        , m_threshold{ m_min_threshold }
    {
    }

    void add(scope_t* scope) { m_scopes.insert(scope); }
    void remove(scope_t* scope) { m_scopes.erase(scope); }
    auto size() const -> std::size_t { return m_scopes.size(); }
    auto is_closed() const -> bool { return m_closed; }

    void ensure_open() const
    {
        if (m_closed)
        {
            throw std::logic_error{ "the stack_manager_t owning this scope has been destroyed" };
        }
    }

    auto should_collect() const -> bool { return !m_collecting && m_scopes.size() >= m_threshold; }

    auto collect() -> std::size_t;
    void close();

private:
    template <class Func>
    static void for_each_scope(const value_t& value, Func&& func);

    static void release(const std::vector<scope_t*>& scopes);

    std::unordered_set<scope_t*> m_scopes;
    std::size_t m_min_threshold;
    std::size_t m_threshold;
    bool m_collecting = false;
    bool m_closed = false;
};

inline scope_t::~scope_t()
{
    if (registry)
    {
        registry->remove(this);
    }
}

}  // namespace evaluate_detail

class stack_t;

// Owns the lifetime of all scopes created through it. Scopes that are no longer reachable
// from outside (a stack_t handle or a closure value held by the program) are freed
// automatically once the number of scopes grows past a threshold, or on collect().
// Destroying the manager releases every scope it owns; any surviving handles then
// throw std::logic_error when used. Not thread-safe.
class stack_manager_t
{
public:
    static constexpr std::size_t default_collection_threshold = 1024;

    explicit stack_manager_t(std::size_t collection_threshold = default_collection_threshold)
        : m_registry{ std::make_shared<evaluate_detail::scope_registry_t>(collection_threshold) }
    {
    }

    stack_manager_t(const stack_manager_t&) = delete;
    stack_manager_t& operator=(const stack_manager_t&) = delete;

    ~stack_manager_t() { m_registry->close(); }

    // Frees unreachable scopes now and returns how many were freed.
    auto collect() -> std::size_t { return m_registry->collect(); }

    auto scope_count() const -> std::size_t { return m_registry->size(); }

    // The manager used by stack_t{ nullptr }; one per thread, released at thread exit.
    static auto default_instance() -> stack_manager_t&
    {
        thread_local stack_manager_t instance;
        return instance;
    }

private:
    friend class stack_t;

    std::shared_ptr<evaluate_detail::scope_registry_t> m_registry;
};

// A handle to a lexical scope. Copies share the same scope, and each scope keeps its
// outer scopes alive, so closures can safely outlive the call that created them.
class stack_t
{
public:
    using frame_type = evaluate_detail::frame_type;

    explicit stack_t(stack_manager_t& manager, frame_type frame = {})
        : stack_t{ manager.m_registry, std::move(frame), nullptr }
    {
    }

    stack_t(frame_type frame, const stack_t* outer)
        : stack_t{ outer ? outer->m_scope->registry : stack_manager_t::default_instance().m_registry,
                   std::move(frame),
                   outer ? outer->m_scope : nullptr }
    {
    }

    stack_t(const stack_t* outer) : stack_t{ frame_type{}, outer } { }

    const value_t& insert(const symbol_t& symbol, const value_t& v)
    {
        m_scope->registry->ensure_open();
        m_scope->frame.emplace(symbol, v);
        return v;
    }

    const value_t& get(const symbol_t& symbol) const
    {
        m_scope->registry->ensure_open();
        for (const evaluate_detail::scope_t* scope = m_scope.get(); scope; scope = scope->outer.get())
        {
            const auto iter = scope->frame.find(symbol);
            if (iter != scope->frame.end())
            {
                return iter->second;
            }
        }

        throw std::runtime_error{ str("Unrecognized symbol `", symbol, "`") };
    }

    const value_t& operator[](const symbol_t& symbol) const { return get(symbol); }

private:
    friend class evaluate_detail::scope_registry_t;

    stack_t(
        const std::shared_ptr<evaluate_detail::scope_registry_t>& registry,
        frame_type frame,
        std::shared_ptr<evaluate_detail::scope_t> outer)
    {
        registry->ensure_open();
        if (registry->should_collect())
        {
            registry->collect();
        }
        m_scope = std::make_shared<evaluate_detail::scope_t>(registry, std::move(outer), std::move(frame));
        registry->add(m_scope.get());
    }

    std::shared_ptr<evaluate_detail::scope_t> m_scope;
};

inline auto make_standard_environment(stack_manager_t& manager = stack_manager_t::default_instance()) -> stack_t
{
    stack_t environment{ manager };
    environment.insert(
        symbol_t{ "+" },
        evaluate_detail::arithmetic_callable(evaluate_detail::arithmetic_operator_t::addition, "+"));
    environment.insert(
        symbol_t{ "-" },
        evaluate_detail::arithmetic_callable(evaluate_detail::arithmetic_operator_t::subtraction, "-"));
    environment.insert(
        symbol_t{ "*" },
        evaluate_detail::arithmetic_callable(evaluate_detail::arithmetic_operator_t::multiplication, "*"));
    environment.insert(
        symbol_t{ "/" },
        evaluate_detail::arithmetic_callable(evaluate_detail::arithmetic_operator_t::division, "/"));
    environment.insert(symbol_t{ "partial" }, evaluate_detail::partial_callable());
    environment.insert(symbol_t{ "comp" }, evaluate_detail::comp_callable());
    return environment;
}

constexpr inline struct evaluate_fn
{
    auto operator()(const value_t& value, stack_t& stack) const -> value_t { return eval(value, stack); }

private:
    friend class evaluate_detail::scope_registry_t;

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
        stack_t stack;

        auto operator()(const std::vector<value_t>& args) -> value_t
        {
            auto new_stack = stack_t{ &stack };

            for (const overload_t& overload : overloads)
            {
                const auto [mandatory, maybe_variadic] = overload.params();

                if (args.size() == mandatory.size() && !maybe_variadic)
                {
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        new_stack.insert(mandatory.at(i), args.at(i));
                    }
                    return self.eval_block(overload.body, new_stack);
                }
                if (args.size() > mandatory.size() && maybe_variadic)
                {
                    list_t tail;
                    for (std::size_t i = 0; i < args.size(); ++i)
                    {
                        if (i < mandatory.size())
                        {
                            new_stack.insert(mandatory.at(i), args.at(i));
                        }
                        else
                        {
                            tail.push_back(args.at(i));
                        }
                    }

                    new_stack.insert(*maybe_variadic, tail);
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
        auto new_stack = stack_t{ stack_t::frame_type{}, &stack };
        for (std::size_t i = 0; i < bindings.size(); i += 2)
        {
            new_stack.insert(*bindings.at(i + 0).if_symbol(), do_eval(bindings.at(i + 1), new_stack));
        }
        return eval_block({ input.begin() + 1, input.end() }, new_stack);
    }

    auto eval_def(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return stack.insert(*input.at(0).if_symbol(), do_eval(input.at(1), stack));
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
        return callable_t{ clojure_t{ *this, std::move(overloads), stack } };
    }

    auto eval_fn(const std::vector<value_t>& input, stack_t& stack) const -> value_t { return eval_callable(input, stack); }

    auto eval_defn(const std::vector<value_t>& input, stack_t& stack) const -> value_t
    {
        return stack.insert(*input.at(0).if_symbol(), eval_callable({ input.begin() + 1, input.end() }, stack));
    }

    auto eval_boolean(const value_t& value, stack_t& stack) const -> bool { return *do_eval(value, stack).if_boolean(); }

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
        const callable_t callable = *do_eval(head, stack).if_callable();
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

namespace evaluate_detail
{

// Calls func for every scope captured by closures nested anywhere inside value.
// Callables not created by the evaluator are opaque; scopes they capture are treated as
// externally referenced, which is safe but keeps those scopes alive.
template <class Func>
void scope_registry_t::for_each_scope(const value_t& value, Func&& func)
{
    const auto visit_all = [&](const auto& items)
    {
        for (const value_t& item : items)
        {
            for_each_scope(item, func);
        }
    };

    if (const auto vector = value.if_vector())
    {
        visit_all(*vector);
    }
    else if (const auto list = value.if_list())
    {
        visit_all(*list);
    }
    else if (const auto set = value.if_set())
    {
        visit_all(*set);
    }
    else if (const auto map = value.if_map())
    {
        for (const auto& [key, val] : *map)
        {
            for_each_scope(key, func);
            for_each_scope(val, func);
        }
    }
    else if (const auto tagged = value.if_tagged_element())
    {
        for_each_scope(tagged->element(), func);
    }
    else if (const auto quoted = value.if_quoted_element())
    {
        for_each_scope(quoted->element(), func);
    }
    else if (const auto callable = value.if_callable())
    {
        if (const auto closure = callable->m_function.target<evaluate_fn::clojure_t>())
        {
            func(closure->stack.m_scope.get());
            for (const auto& overload : closure->overloads)
            {
                for_each_scope(overload.parameters, func);
                visit_all(overload.body);
            }
        }
        else if (const auto partial = callable->m_function.target<partial_t>())
        {
            for_each_scope(partial->function, func);
            visit_all(partial->bound_args);
        }
        else if (const auto composition = callable->m_function.target<comp_t>())
        {
            visit_all(composition->functions);
        }
    }
}

// Breaks all references held by the given scopes. Values are moved out first and destroyed
// only after every scope has been detached, so no scope is freed while being processed.
inline void scope_registry_t::release(const std::vector<scope_t*>& scopes)
{
    std::vector<frame_type> frames;
    std::vector<std::shared_ptr<scope_t>> outers;
    frames.reserve(scopes.size());
    outers.reserve(scopes.size());
    for (scope_t* scope : scopes)
    {
        frames.push_back(std::move(scope->frame));
        scope->frame.clear();
        outers.push_back(std::move(scope->outer));
        scope->outer.reset();
    }
}

// Cycle collection by trial deletion: a scope whose strong reference count exceeds the
// number of references coming from managed scopes is held from outside (a handle or a
// value on the C++ stack) and is a root. Everything not reachable from a root is garbage.
inline auto scope_registry_t::collect() -> std::size_t
{
    if (m_collecting)
    {
        return 0;
    }
    m_collecting = true;

    const std::vector<scope_t*> scopes(m_scopes.begin(), m_scopes.end());

    std::unordered_map<const scope_t*, long> external_refs;
    external_refs.reserve(scopes.size());
    for (const scope_t* scope : scopes)
    {
        external_refs.emplace(scope, scope->weak_from_this().use_count());
    }

    const auto drop_internal_ref = [&](const scope_t* target)
    {
        if (const auto iter = external_refs.find(target); iter != external_refs.end())
        {
            --iter->second;
        }
    };

    for (const scope_t* scope : scopes)
    {
        drop_internal_ref(scope->outer.get());
        for (const auto& entry : scope->frame)
        {
            for_each_scope(entry.second, drop_internal_ref);
        }
    }

    std::unordered_set<const scope_t*> reachable;
    std::vector<const scope_t*> pending;
    for (const auto& [scope, refs] : external_refs)
    {
        if (refs > 0)
        {
            pending.push_back(scope);
        }
    }

    while (!pending.empty())
    {
        const scope_t* scope = pending.back();
        pending.pop_back();
        if (!scope || !reachable.insert(scope).second)
        {
            continue;
        }
        pending.push_back(scope->outer.get());
        for (const auto& entry : scope->frame)
        {
            for_each_scope(entry.second, [&](const scope_t* target) { pending.push_back(target); });
        }
    }

    std::vector<scope_t*> garbage;
    for (scope_t* scope : scopes)
    {
        if (reachable.count(scope) == 0)
        {
            garbage.push_back(scope);
        }
    }

    release(garbage);

    m_threshold = std::max(m_min_threshold, 2 * m_scopes.size());
    m_collecting = false;
    return garbage.size();
}

inline void scope_registry_t::close()
{
    m_closed = true;
    m_collecting = true;
    release(std::vector<scope_t*>(m_scopes.begin(), m_scopes.end()));
    m_collecting = false;
}

}  // namespace evaluate_detail

}  // namespace edn
