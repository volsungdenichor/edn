#include <gmock/gmock.h>

#include <edn/evaluate.hpp>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace
{

auto eval(std::string_view expression, edn::stack_t& stack) -> edn::value_t
{
    return edn::evaluate(edn::parse(expression), stack);
}

// Binds `token`, a callable owning a resource, in stack; the returned weak_ptr reports
// whether anything still keeps the resource alive.
auto insert_token(edn::stack_t& stack) -> std::weak_ptr<int>
{
    auto resource = std::make_shared<int>(42);
    stack.insert(
        edn::symbol_t{ "token" },
        edn::callable_t{ [resource](const std::vector<edn::value_t>&) -> edn::value_t { return *resource; } });
    return resource;
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

TEST(evaluate, closure_returned_from_function_keeps_its_arguments_alive)
{
    edn::stack_t stack{ nullptr };

    eval("(defn constantly [x] (fn [] x))", stack);
    eval("(def answer (constantly 42))", stack);
    eval("(def other (constantly 7))", stack);

    EXPECT_THAT(eval("(answer)", stack), 42);
    EXPECT_THAT(eval("(other)", stack), 7);
}

TEST(evaluate, curried_function_captures_each_argument)
{
    auto environment = edn::make_standard_environment();

    eval("(defn adder [x] (fn [y] (fn [z] (+ x y z))))", environment);

    EXPECT_THAT(eval("(((adder 1) 2) 3)", environment), 6);
}

TEST(evaluate, closure_outlives_let_scope)
{
    edn::stack_t stack{ nullptr };

    eval("(def get-x (let [x 5] (fn [] x)))", stack);

    EXPECT_THAT(eval("(get-x)", stack), 5);
}

TEST(evaluate, closure_outlives_the_environment_handle_it_was_created_in)
{
    edn::value_t closure;
    {
        edn::stack_t stack{ nullptr };
        closure = eval("(let [x 9] (fn [] x))", stack);
    }

    ASSERT_NE(closure.if_callable(), nullptr);
    EXPECT_THAT((*closure.if_callable())({}), 9);
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

TEST(evaluate, standard_environment_supports_addition)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("(+)", environment), 0);
    EXPECT_THAT(eval("(+ 4)", environment), 4);
    EXPECT_THAT(eval("(+ 1 2 3)", environment), 6);
    EXPECT_THAT(eval("(+ 1 2.5)", environment), 3.5);
}

TEST(evaluate, standard_environment_supports_subtraction)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("(- 4)", environment), -4);
    EXPECT_THAT(eval("(- 10 3 2)", environment), 5);
    EXPECT_THAT(eval("(- 5.5 2)", environment), 3.5);
}

TEST(evaluate, standard_environment_supports_multiplication)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("(*)", environment), 1);
    EXPECT_THAT(eval("(* 4)", environment), 4);
    EXPECT_THAT(eval("(* 2 3 4)", environment), 24);
    EXPECT_THAT(eval("(* 2 1.5)", environment), 3.0);
}

TEST(evaluate, standard_environment_supports_division)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("(/ 2)", environment), 0.5);
    EXPECT_THAT(eval("(/ 12 3 2)", environment), 2);
    EXPECT_THAT(eval("(/ 7 2)", environment), 3.5);
}

TEST(evaluate, arithmetic_operators_work_in_nested_expressions)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("(* (+ 2 3) (- 10 6))", environment), 20);
}

TEST(evaluate, arithmetic_promotes_integer_overflow_to_floating_point)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("(+ 2147483647 1)", environment), 2147483648.0);
    EXPECT_THAT(eval("(- -2147483647 2)", environment), -2147483649.0);
    EXPECT_THAT(eval("(* 50000 50000)", environment), 2500000000.0);
    EXPECT_THAT(eval("(- (- -2147483647 1))", environment), 2147483648.0);
    EXPECT_THAT(eval("(/ (- -2147483647 1) -1)", environment), 2147483648.0);
}

TEST(evaluate, arithmetic_operators_reject_invalid_arguments)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(
        [&]() { return eval("(+ 1 true)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires numeric arguments")));
    EXPECT_THAT(
        [&]() { return eval("(-)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires at least one argument")));
    EXPECT_THAT(
        [&]() { return eval("(/ 1 0)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("division by zero")));
}

TEST(evaluate, partial_binds_leading_arguments)
{
    auto environment = edn::make_standard_environment();
    eval("(defn affine [a b x] (+ (* a x) b))", environment);

    EXPECT_THAT(eval("((partial + 10) 1 2)", environment), 13);
    EXPECT_THAT(eval("((partial - 10) 3)", environment), 7);
    EXPECT_THAT(eval("((partial affine 2 1) 5)", environment), 11);
    EXPECT_THAT(eval("((partial (partial affine 2) 1) 5)", environment), 11);
    EXPECT_THAT(eval("((partial affine 2 1 5))", environment), 11);
    EXPECT_THAT(eval("((partial +) 1 2)", environment), 3);
}

TEST(evaluate, partial_result_can_be_stored_and_reused)
{
    auto environment = edn::make_standard_environment();
    eval("(def add5 (partial + 5))", environment);

    EXPECT_THAT(eval("(add5 1)", environment), 6);
    EXPECT_THAT(eval("(add5 (add5 0))", environment), 10);
}

TEST(evaluate, comp_applies_functions_right_to_left)
{
    auto environment = edn::make_standard_environment();
    eval("(defn inc [x] (+ x 1))", environment);
    eval("(defn double [x] (* x 2))", environment);

    EXPECT_THAT(eval("((comp inc double) 5)", environment), 11);
    EXPECT_THAT(eval("((comp double inc) 5)", environment), 12);
    EXPECT_THAT(eval("((comp inc inc double) 5)", environment), 12);
    EXPECT_THAT(eval("((comp inc) 5)", environment), 6);
    EXPECT_THAT(eval("((comp) 5)", environment), 5);
}

TEST(evaluate, comp_passes_all_arguments_to_rightmost_function)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(eval("((comp (partial * 2) +) 1 2 3)", environment), 12);
}

TEST(evaluate, partial_and_comp_reject_invalid_arguments)
{
    auto environment = edn::make_standard_environment();

    EXPECT_THAT(
        [&]() { return eval("(partial)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("`partial` requires at least one argument")));
    EXPECT_THAT(
        [&]() { return eval("(partial 1 2)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("`partial` requires callable arguments")));
    EXPECT_THAT(
        [&]() { return eval("(comp + 1)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("`comp` requires callable arguments")));
    EXPECT_THAT(
        [&]() { return eval("((comp) 1 2)", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires exactly one argument")));
}

TEST(scope_lifetime, environment_frees_everything_it_defined)
{
    std::weak_ptr<int> token;
    {
        auto environment = edn::make_standard_environment();
        token = insert_token(environment);
        eval("(defn call-token [] (token))", environment);
        eval("(def held (let [t token] (fn [] (t))))", environment);
        eval("(def add-token (partial + (token)))", environment);
        eval("(def composed (comp call-token held))", environment);
        EXPECT_THAT(eval("(add-token (held) (call-token))", environment), 126);
        EXPECT_FALSE(token.expired());
    }

    EXPECT_TRUE(token.expired());
}

TEST(scope_lifetime, let_scopes_are_freed_after_evaluation)
{
    edn::stack_t stack;
    const auto token = insert_token(stack);

    EXPECT_THAT(eval("(let [t token f (fn [] (t)) g (fn [] (f))] (g))", stack), 42);
    EXPECT_THAT(token.use_count(), 1);
}

TEST(scope_lifetime, function_scopes_are_freed_after_evaluation)
{
    edn::stack_t stack;
    const auto token = insert_token(stack);
    eval("(defn wrap [x] (fn [] (x)))", stack);

    EXPECT_THAT(eval("((wrap token))", stack), 42);
    EXPECT_THAT(token.use_count(), 1);
}

TEST(scope_lifetime, closure_held_from_cpp_keeps_its_locals_alive)
{
    edn::stack_t stack;
    const auto token = insert_token(stack);
    edn::value_t closure = eval("(let [t token] (fn [] (t)))", stack);
    EXPECT_THAT(token.use_count(), 2);

    ASSERT_NE(closure.if_callable(), nullptr);
    EXPECT_THAT((*closure.if_callable())({}), 42);

    closure = edn::nil;
    EXPECT_THAT(token.use_count(), 1);
}

TEST(scope_lifetime, recursion_through_defn_works)
{
    auto environment = edn::make_standard_environment();
    eval("(defn twice ([f] (twice f 1)) ([f x] (f (f x))))", environment);

    EXPECT_THAT(eval("(twice (partial + 1))", environment), 3);
}

TEST(scope_lifetime, def_inside_a_function_defines_a_global)
{
    edn::stack_t stack;
    eval("(defn setup [x] (def answer x))", stack);
    eval("(setup 42)", stack);

    EXPECT_THAT(eval("answer", stack), 42);
}

TEST(scope_lifetime, let_binding_cannot_refer_to_itself)
{
    edn::stack_t stack;

    EXPECT_THAT(
        [&]() { return eval("(let [f (fn [] (f))] (f))", stack); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Unrecognized symbol `f`")));
}

TEST(scope_lifetime, closure_using_globals_throws_after_environment_is_destroyed)
{
    edn::value_t uses_global;
    edn::value_t uses_locals;
    {
        auto environment = edn::make_standard_environment();
        uses_global = eval("(let [x 1] (fn [] (+ x 1)))", environment);
        uses_locals = eval("(let [x 1] (fn [] x))", environment);
    }

    ASSERT_THAT(uses_locals.if_callable(), testing::NotNull());
    EXPECT_THAT((*uses_locals.if_callable())({}), 1);
    ASSERT_NE(uses_global.if_callable(), nullptr);
    EXPECT_THAT(
        [&]() { return (*uses_global.if_callable())({}); },
        testing::ThrowsMessage<std::logic_error>(testing::HasSubstr("root environment of this scope has been destroyed")));
}
