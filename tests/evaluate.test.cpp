#include <gmock/gmock.h>

#include <edn/evaluate.hpp>

TEST(evaluate, value_evaluates_to_itself)
{
    edn::stack_t stack{ nullptr };
    EXPECT_THAT(edn::evaluate(3, stack), 3);
}
