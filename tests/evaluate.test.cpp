#include <gmock/gmock.h>

#include <edn/evaluate.hpp>
#include <stdexcept>

namespace
{

auto eval(std::string_view expression, edn::stack_t& stack) -> edn::value_t
{
    return edn::evaluate(edn::parse(expression), stack);
}

}  // namespace

TEST(evaluate, value_evaluates_to_itself)
{
    edn::stack_t stack{ nullptr };
    EXPECT_THAT(edn::evaluate(3, stack), 3);
}

TEST(evaluate, non_executable_values_evaluate_to_themselves)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(edn::evaluate(edn::nil, stack), edn::nil);
    EXPECT_THAT(edn::evaluate(true, stack), true);
    EXPECT_THAT(edn::evaluate("hello", stack), "hello");
    EXPECT_THAT(edn::evaluate(edn::keyword_t{ "name" }, stack), edn::keyword_t{ "name" });
}

TEST(evaluate, resolves_symbols_from_current_and_outer_frames)
{
    edn::stack_t outer{ nullptr };
    outer.insert(edn::symbol_t{ "outer" }, 42);
    edn::stack_t inner{ &outer };
    inner.insert(edn::symbol_t{ "inner" }, 7);

    EXPECT_THAT(eval("outer", inner), 42);
    EXPECT_THAT(eval("inner", inner), 7);
}

TEST(evaluate, evaluates_symbols_in_nested_collections)
{
    edn::stack_t stack{ nullptr };
    stack.insert(edn::symbol_t{ "x" }, 9);

    EXPECT_THAT(eval("[x #{x} {x x}]", stack), edn::parse("[9 #{9} {9 9}]"));
}

TEST(evaluate, quoted_values_are_not_evaluated)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("'(+ missing 2)", stack), edn::parse("(+ missing 2)"));
}

TEST(evaluate, empty_list_evaluates_to_nil)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("()", stack), edn::nil);
}

TEST(evaluate, let_bindings_are_sequential_and_scoped)
{
    edn::stack_t stack{ nullptr };
    stack.insert(edn::symbol_t{ "x" }, 1);

    EXPECT_THAT(eval("(let [x 2 y x] [x y])", stack), edn::parse("[2 2]"));
    EXPECT_THAT(eval("x", stack), 1);
}

TEST(evaluate, def_adds_a_value_to_the_current_frame)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("(def answer 42)", stack), 42);
    EXPECT_THAT(eval("answer", stack), 42);
}

TEST(evaluate, do_returns_the_last_expression)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("(do 1 2 3)", stack), 3);
}

TEST(evaluate, if_evaluates_only_the_selected_branch)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("(if true 1 missing)", stack), 1);
    EXPECT_THAT(eval("(if false missing 2)", stack), 2);
}

TEST(evaluate, cond_selects_first_matching_clause_or_else)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("(cond false 1 true 2 :else 3)", stack), 2);
    EXPECT_THAT(eval("(cond false 1 :else 3)", stack), 3);
    EXPECT_THAT(eval("(cond false 1)", stack), edn::nil);
}

TEST(evaluate, invokes_function_and_captures_lexical_scope)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("((fn [x] [x]) 42)", stack), edn::parse("[42]"));
    EXPECT_THAT(eval("(let [x 1 f (fn [y] [x y])] (f 2))", stack), edn::parse("[1 2]"));
}

TEST(evaluate, selects_function_overload_by_arity)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("((fn ([x] x) ([x y] y)) 1)", stack), 1);
    EXPECT_THAT(eval("((fn ([x] x) ([x y] y)) 1 2)", stack), 2);
}

TEST(evaluate, binds_variadic_arguments_as_a_list)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(eval("((fn [head & tail] [head tail]) 1 2 3)", stack), edn::parse("[1 (2 3)]"));
}

TEST(evaluate, defn_defines_a_callable_in_the_current_frame)
{
    edn::stack_t stack{ nullptr };

    eval("(defn identity [x] x)", stack);

    EXPECT_THAT(eval("(identity 42)", stack), 42);
}

TEST(evaluate, throws_for_unresolved_symbols)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(
        [&]() { return eval("missing", stack); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Unrecognized symbol `missing`")));
}

TEST(evaluate, throws_when_no_function_overload_matches)
{
    edn::stack_t stack{ nullptr };

    EXPECT_THAT(
        [&]() { return eval("((fn [x] x) 1 2)", stack); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("could not resolve function overload for 2 arg(s)")));
}
