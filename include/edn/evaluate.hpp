#pragma once

#pragma once

#include <edn/edn.hpp>
#include <map>
#include <numeric>
#include <optional>

namespace edn
{

struct stack_t
{
    using frame_type = std::map<symbol_t, value_t>;
    frame_type m_frame;
    stack_t* m_outer;

    stack_t(frame_type frame, stack_t* outer) : m_frame{ std::move(frame) }, m_outer{ outer } { }

    stack_t(stack_t* outer) : stack_t{ frame_type{}, outer } { }

    const value_t& insert(const symbol_t& symbol, const value_t& v)
    {
        m_frame.emplace(symbol, v);
        return v;
    }

    const value_t& get(const symbol_t& symbol) const
    {
        const auto iter = m_frame.find(symbol);
        if (iter != m_frame.end())
        {
            return iter->second;
        }
        if (m_outer)
        {
            return m_outer->get(symbol);
        }

        throw std::runtime_error{ str("Unrecognized symbol `", symbol, "`") };
    }

    const value_t& operator[](const symbol_t& symbol) const { return get(symbol); }
};

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
        stack_t& stack;

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
                overloads.push_back(create_overload(std::vector<value_t>{ *v.if_list() }));
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

}  // namespace edn
