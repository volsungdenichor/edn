set(TARGET_NAME edn-tests)

set(UNIT_TEST_SOURCE_LIST
    edn.test.cpp
    parse.test.cpp
    evaluate.test.cpp
)

Include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(${TARGET_NAME} ${UNIT_TEST_SOURCE_LIST})
target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(${TARGET_NAME} PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

edn_set_strict_warnings(${TARGET_NAME})

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})
