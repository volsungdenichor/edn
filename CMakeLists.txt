cmake_minimum_required(VERSION 3.14)

project(edn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(edn_set_strict_warnings TARGET_NAME)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /W4
            /WX
            /permissive-
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Woverloaded-virtual
            -Wunused
        )

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${TARGET_NAME} PRIVATE
                -Wmost
            )
        endif()

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_compile_options(${TARGET_NAME} PRIVATE
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wno-maybe-uninitialized
            )
        endif()
    endif()
endfunction()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
