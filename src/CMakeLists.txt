set(TARGET_NAME edn)

add_executable(${TARGET_NAME} main.cpp)

target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

# Apply strict compiler warnings
edn_set_strict_warnings(${TARGET_NAME})
