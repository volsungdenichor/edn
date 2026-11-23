#include <gmock/gmock.h>

#include <edn/edn.hpp>
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

TEST(evaluate, thread_last_builds_collection_pipelines)
{
    auto environment = edn::make_standard_environment();
    EXPECT_THAT(eval("(->> [1 2 3 4] (mapv (partial * 10)) (filterv (fn [x] (> x 20))) (reduce + 0))", environment), 70);
    EXPECT_THAT(eval("(->> 10 (- 100) (/ 180))", environment), 2);
    EXPECT_THAT(eval("(->> 42 vector (list :answer))", environment), edn::parse("(:answer [42])"));
    EXPECT_THAT(eval("(->> 3 ((partial + 2)))", environment), 5);
    EXPECT_THAT(eval("(let [x 4] (->> (->> x (+ 1)) (* 2)))", environment), 10);
}

TEST(evaluate, thread_last_expands_before_evaluation)
{
    edn::stack_t stack;
    EXPECT_THAT(eval("(->> 42)", stack), 42);
    EXPECT_THAT(eval("(->> nil)", stack), edn::nil);
    EXPECT_THAT(eval("(->> missing (and false))", stack), false);
    EXPECT_THAT(eval("(->> missing (or :ready))", stack), edn::keyword_t{ "ready" });
    EXPECT_THAT(eval("(->> missing quote)", stack), edn::symbol_t{ "missing" });
    EXPECT_THAT(eval("(->> '(unbound 1) ((fn [x] x)))", stack), edn::parse("(unbound 1)"));
    EXPECT_THAT(
        [&]() { return eval("(->>)", stack); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires an initial expression")));
    EXPECT_THAT(
        [&]() { return eval("(->> 1 missing)", stack); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Unrecognized symbol")));
}

TEST(evaluate, thread_last_evaluates_each_expression_once_in_nested_call_order)
{
    auto environment = edn::make_standard_environment();
    std::vector<edn::value_t> calls;
    environment.insert(
        edn::symbol_t{ "record" },
        edn::callable_t{ [&calls](const std::vector<edn::value_t>& args) -> edn::value_t
                         {
                             calls.push_back(args.at(0));
                             return args.at(0);
                         } });
    EXPECT_THAT(eval("(->> (record 1) (+ (record 2)) (* (record 3)))", environment), 9);
    EXPECT_THAT(calls, testing::ElementsAre(edn::value_t{ 3 }, edn::value_t{ 2 }, edn::value_t{ 1 }));
}

TEST(evaluate, collection_constructors_create_empty_and_populated_collections)
{
    auto environment = edn::make_standard_environment();
    struct test_case_t
    {
        const char* expression;
        const char* expected;
    };
    for (const auto& test : { test_case_t{ "(vector)", "[]" },
                              test_case_t{ "(list)", "()" },
                              test_case_t{ "(hash-map)", "{}" },
                              test_case_t{ "(hash-set)", "#{}" },
                              test_case_t{ "(vector 1 (+ 1 1) :a)", "[1 2 :a]" },
                              test_case_t{ "(list 1 (+ 1 1) :a)", "(1 2 :a)" },
                              test_case_t{ "(hash-set 3 1 3 2)", "#{1 2 3}" },
                              test_case_t{ "(hash-map :a 1 :b 2 :a 3)", "{:a 3 :b 2}" },
                              test_case_t{ "(vector (list 1) (hash-map :a (vector 2)))", "[(1) {:a [2]}]" },
                              test_case_t{ "((partial vector 1) 2)", "[1 2]" } })
    {
        SCOPED_TRACE(test.expression);
        EXPECT_THAT(eval(test.expression, environment), edn::parse(test.expected));
    }
    for (const auto expression : { "(hash-map :a)", "(hash-map :a 1 :b)" })
    {
        EXPECT_THAT(
            [&]() { return eval(expression, environment); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("even number of arguments")));
    }
}

TEST(evaluate, mapv_maps_lists_vectors_and_multiple_inputs)
{
    auto environment = edn::make_standard_environment();
    EXPECT_THAT(eval("(mapv (partial + 1) [1 2 3])", environment), edn::parse("[2 3 4]"));
    EXPECT_THAT(eval("(mapv (fn [x] (* x x)) '(1 2 3))", environment), edn::parse("[1 4 9]"));
    EXPECT_THAT(eval("(mapv + [1 2 3] '(10 20))", environment), edn::parse("[11 22]"));
    EXPECT_THAT(eval("(mapv vector [1 2] '(3 4) [5 6 7])", environment), edn::parse("[[1 3 5] [2 4 6]]"));
    for (const auto expression : { "(mapv + [])", "(mapv + nil)", "(mapv + [1] nil)", "(mapv + [] [1])" })
    {
        EXPECT_THAT(eval(expression, environment), edn::parse("[]"));
    }
    EXPECT_THAT(eval("(mapv (fn [x] nil) [1 2])", environment), edn::parse("[nil nil]"));
}

TEST(evaluate, filterv_preserves_order_and_uses_clojure_truthiness)
{
    auto environment = edn::make_standard_environment();
    EXPECT_THAT(eval("(filterv (fn [x] (> x 1)) '(3 1 2))", environment), edn::parse("[3 2]"));
    EXPECT_THAT(eval("(filterv number? [1 :a 2.5 nil])", environment), edn::parse("[1 2.5]"));
    EXPECT_THAT(eval("(filterv (fn [x] x) [nil false 0 [] :a])", environment), edn::parse("[0 [] :a]"));
    EXPECT_THAT(eval("(filterv (fn [x] false) [1 2])", environment), edn::parse("[]"));
    EXPECT_THAT(eval("(filterv (fn [x] 0) [1 2])", environment), edn::parse("[1 2]"));
    EXPECT_THAT(eval("(filterv number? [])", environment), edn::parse("[]"));
    EXPECT_THAT(eval("(filterv number? nil)", environment), edn::parse("[]"));
}

TEST(evaluate, reduce_supports_initial_values_and_left_to_right_accumulation)
{
    auto environment = edn::make_standard_environment();
    EXPECT_THAT(eval("(reduce + [1 2 3])", environment), 6);
    EXPECT_THAT(eval("(reduce + 10 '(1 2 3))", environment), 16);
    EXPECT_THAT(eval("(reduce - [10 3 2])", environment), 5);
    EXPECT_THAT(eval("(reduce - 10 [3 2])", environment), 5);
    EXPECT_THAT(eval("(reduce vector nil [1 2])", environment), edn::parse("[[nil 1] 2]"));
    EXPECT_THAT(eval("(reduce + [])", environment), 0);
    EXPECT_THAT(eval("(reduce * nil)", environment), 1);
    EXPECT_THAT(eval("(reduce + 42 [])", environment), 42);
    EXPECT_THAT(eval("(reduce + :initial nil)", environment), edn::keyword_t{ "initial" });
    EXPECT_THAT(eval("(reduce (fn [] :empty) [])", environment), edn::keyword_t{ "empty" });
    EXPECT_THAT(eval("(reduce (fn [] :unused) [42])", environment), 42);
    EXPECT_THAT(
        [&]() { return eval("(reduce (fn [a b] a) [])", environment); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("could not resolve function overload")));
}

TEST(evaluate, eager_sequence_functions_validate_arity_functions_and_inputs)
{
    auto environment = edn::make_standard_environment();
    for (const auto expression : { "(mapv)",
                                   "(mapv +)",
                                   "(filterv)",
                                   "(filterv +)",
                                   "(filterv + [] [])",
                                   "(reduce)",
                                   "(reduce +)",
                                   "(reduce + 0 [] [])" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(
            [&]() { return eval(expression, environment); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires")));
    }
    for (const auto expression : { "(mapv 1 [])", "(filterv 1 nil)", "(reduce 1 0 [])" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(
            [&]() { return eval(expression, environment); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires callable arguments")));
    }
    for (const auto expression : { "(mapv + 1)",
                                   "(mapv + [] {})",
                                   "(filterv number? #{1})",
                                   "(filterv number? \"abc\")",
                                   "(reduce + :a)",
                                   "(reduce + 0 {})" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(
            [&]() { return eval(expression, environment); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires a list, vector, or nil")));
    }
    for (const auto expression :
         { "(mapv (fn [x] missing) [1])", "(filterv (fn [x] missing) [1])", "(reduce (fn [a b] missing) [1 2])" })
    {
        EXPECT_THAT(
            [&]() { return eval(expression, environment); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Unrecognized symbol")));
    }
}

TEST(evaluate, sequence_callbacks_are_eager_and_invoked_once_in_order)
{
    auto environment = edn::make_standard_environment();
    std::vector<edn::value_t> calls;
    environment.insert(
        edn::symbol_t{ "record" },
        edn::callable_t{ [&calls](const std::vector<edn::value_t>& args) -> edn::value_t
                         {
                             calls.push_back(args.at(0));
                             return args.at(0);
                         } });
    EXPECT_THAT(eval("(mapv record [3 1 2])", environment), edn::parse("[3 1 2]"));
    EXPECT_THAT(calls, testing::ElementsAre(edn::value_t{ 3 }, edn::value_t{ 1 }, edn::value_t{ 2 }));
    calls.clear();
    EXPECT_THAT(eval("(filterv record [nil 0 false])", environment), edn::parse("[0]"));
    EXPECT_THAT(calls, testing::ElementsAre(edn::value_t{ edn::nil }, edn::value_t{ 0 }, edn::value_t{ false }));
    calls.clear();
    eval("(mapv record nil)", environment);
    eval("(filterv record [])", environment);
    eval("(reduce record 1 [])", environment);
    EXPECT_TRUE(calls.empty());
}

TEST(scope_lifetime, eager_results_keep_closures_alive_without_retaining_temporary_scopes)
{
    auto environment = edn::make_standard_environment();
    const auto token = insert_token(environment);
    {
        auto result = eval("(mapv (fn [t] (fn [] (t))) (vector token))", environment);
        ASSERT_NE(result.if_vector(), nullptr);
        ASSERT_EQ(result.if_vector()->size(), 1u);
        EXPECT_THAT((*result.if_vector()->front().if_callable())({}), 42);
        EXPECT_THAT(token.use_count(), testing::Gt(1));
    }
    EXPECT_THAT(token.use_count(), 1);
}

TEST(evaluate, numeric_comparisons_check_every_adjacent_pair)
{
    auto environment = edn::make_standard_environment();
    for (const auto expression : { "(< 1 2 3)",
                                   "(<= 1 1.0 2)",
                                   "(> 3 2.5 1)",
                                   "(>= 3 3.0 1)",
                                   "(< -2147483648 2147483647)",
                                   "(< 2147483647 2147483648.0)",
                                   "(< 1)",
                                   "(<= 1)",
                                   "(> 1)",
                                   "(>= 1)" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(eval(expression, environment), true);
    }
    for (const auto expression : { "(< 1 3 2)", "(< 1 1.0)", "(<= 2 1)", "(> 3 1 2)", "(> 1 1.0)", "(>= 1 2)" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(eval(expression, environment), false);
    }
}

TEST(evaluate, equality_uses_existing_value_equality_and_not_equal_negates_it)
{
    auto environment = edn::make_standard_environment();
    for (const auto expression : { "(= 1)",
                                   "(= nil nil)",
                                   "(= 1 1 1)",
                                   "(= [1 {:a 2}] [1 {:a 2}])",
                                   "(= 'x 'x)",
                                   "(not= 1 1 2)",
                                   "(not= 1 1.0)",
                                   "(not= true 1)" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(eval(expression, environment), true);
    }
    for (const auto expression :
         { "(= 1 1 2)", "(= 1 1.0)", "(= true 1)", "(= nil false)", "(= [1] [2])", "(not= 1)", "(not= 1 1 1)" })
    {
        SCOPED_TRACE(expression);
        EXPECT_THAT(eval(expression, environment), false);
    }
}

TEST(evaluate, comparisons_reject_missing_or_nonnumeric_arguments)
{
    auto environment = edn::make_standard_environment();
    for (const auto name : { "=", "not=", "<", "<=", ">", ">=" })
    {
        SCOPED_TRACE(name);
        EXPECT_THAT(
            [&]() { return eval(edn::str("(", name, ")"), environment); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires at least one argument")));
    }
    for (const auto name : { "<", "<=", ">", ">=" })
    {
        for (const auto arguments : { "true", "1 nil", "2 1 :invalid" })
        {
            SCOPED_TRACE(edn::str(name, " ", arguments));
            EXPECT_THAT(
                [&]() { return eval(edn::str("(", name, " ", arguments, ")"), environment); },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires numeric arguments")));
        }
    }
}

TEST(evaluate, predicates_recognize_types_and_require_one_argument)
{
    auto environment = edn::make_standard_environment();
    struct test_case_t
    {
        const char* name;
        const char* matching;
        const char* nonmatching;
    };
    for (const auto& test : { test_case_t{ "nil?", "nil", "false" },
                              test_case_t{ "boolean?", "false", "nil" },
                              test_case_t{ "true?", "true", "1" },
                              test_case_t{ "false?", "false", "nil" },
                              test_case_t{ "number?", "1.5", "true" },
                              test_case_t{ "integer?", "1", "1.0" },
                              test_case_t{ "float?", "1.0", "1" },
                              test_case_t{ "char?", "\\a", "\"a\"" },
                              test_case_t{ "string?", "\"a\"", ":a" },
                              test_case_t{ "symbol?", "'a", ":a" },
                              test_case_t{ "keyword?", ":a", "'a" },
                              test_case_t{ "list?", "'(1)", "[1]" },
                              test_case_t{ "vector?", "[1]", "'(1)" },
                              test_case_t{ "set?", "#{1}", "[1]" },
                              test_case_t{ "map?", "{:a 1}", "[1]" },
                              test_case_t{ "fn?", "(fn [] nil)", "nil" },
                              test_case_t{ "coll?", "{}", "\"\"" },
                              test_case_t{ "sequential?", "[]", "#{}" },
                              test_case_t{ "not", "nil", "0" } })
    {
        SCOPED_TRACE(test.name);
        EXPECT_THAT(eval(edn::str("(", test.name, " ", test.matching, ")"), environment), true);
        EXPECT_THAT(eval(edn::str("(", test.name, " ", test.nonmatching, ")"), environment), false);
        for (const auto arguments : { "", "nil nil" })
        {
            EXPECT_THAT(
                [&]() { return eval(edn::str("(", test.name, " ", arguments, ")"), environment); },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("requires exactly one argument")));
        }
    }
    EXPECT_THAT(eval("(number? 1)", environment), true);
    EXPECT_THAT(eval("(fn? +)", environment), true);
    EXPECT_THAT(eval("(coll? [])", environment), true);
    EXPECT_THAT(eval("(coll? '())", environment), true);
    EXPECT_THAT(eval("(coll? #{})", environment), true);
    EXPECT_THAT(eval("(coll? nil)", environment), false);
    EXPECT_THAT(eval("(sequential? '())", environment), true);
    EXPECT_THAT(eval("(true? false)", environment), false);
    EXPECT_THAT(eval("(false? true)", environment), false);
}

TEST(evaluate, only_nil_and_false_are_falsey_in_not_if_and_cond)
{
    auto environment = edn::make_standard_environment();
    for (const auto value : { "nil", "false" })
    {
        SCOPED_TRACE(value);
        EXPECT_THAT(eval(edn::str("(not ", value, ")"), environment), true);
        EXPECT_THAT(eval(edn::str("(if ", value, " missing 2)"), environment), 2);
        EXPECT_THAT(eval(edn::str("(cond ", value, " missing :else 2)"), environment), 2);
    }
    for (const auto value : { "true", "0", "0.0", "\"\"", "[]", "'()", "#{}", "{}", ":ready", "(fn [] nil)" })
    {
        SCOPED_TRACE(value);
        EXPECT_THAT(eval(edn::str("(not ", value, ")"), environment), false);
        EXPECT_THAT(eval(edn::str("(if ", value, " 1 missing)"), environment), 1);
        EXPECT_THAT(eval(edn::str("(cond ", value, " 1 :else missing)"), environment), 1);
    }
}

TEST(evaluate, and_or_return_operand_values_and_handle_empty_input)
{
    edn::stack_t stack;
    EXPECT_THAT(eval("(and)", stack), true);
    EXPECT_THAT(eval("(or)", stack), edn::nil);
    EXPECT_THAT(eval("(and 42)", stack), 42);
    EXPECT_THAT(eval("(or false)", stack), false);
    EXPECT_THAT(eval("(and true 1 [2])", stack), edn::parse("[2]"));
    EXPECT_THAT(eval("(and 1 nil missing)", stack), edn::nil);
    EXPECT_THAT(eval("(and 1 false missing)", stack), false);
    EXPECT_THAT(eval("(or nil false 0 missing)", stack), 0);
    EXPECT_THAT(eval("(or nil false)", stack), false);
    EXPECT_THAT(eval("(or false nil)", stack), edn::nil);
    EXPECT_THAT(eval("(or [] missing)", stack), edn::parse("[]"));
    EXPECT_THAT(eval("(and (or nil 1) 2)", stack), 2);
}

TEST(evaluate, and_or_evaluate_once_in_order_and_skip_side_effects)
{
    edn::stack_t stack;
    int calls = 0;
    stack.insert(
        edn::symbol_t{ "tick" },
        edn::callable_t{ [&calls](const std::vector<edn::value_t>&) -> edn::value_t { return ++calls; } });

    EXPECT_THAT(eval("(and (tick) (tick) false (tick))", stack), false);
    EXPECT_THAT(calls, 2);
    EXPECT_THAT(eval("(or nil (tick) (tick))", stack), 3);
    EXPECT_THAT(calls, 3);
    EXPECT_THAT(eval("(and false (def skipped 1))", stack), false);
    EXPECT_THAT(
        [&]() { return eval("skipped", stack); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Unrecognized symbol")));
    for (const auto expression : { "(and true missing)", "(or nil missing)" })
    {
        EXPECT_THAT(
            [&]() { return eval(expression, stack); },
            testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Unrecognized symbol")));
    }
}

TEST(evaluate, comparisons_and_predicates_work_as_first_class_functions)
{
    auto environment = edn::make_standard_environment();
    EXPECT_THAT(eval("((partial < 1) 2 3)", environment), true);
    EXPECT_THAT(eval("((comp not nil?) 42)", environment), true);
    eval("(defn factorial [n] (if (<= n 1) 1 (* n (factorial (- n 1)))))", environment);
    EXPECT_THAT(eval("(factorial 6)", environment), 720);
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
