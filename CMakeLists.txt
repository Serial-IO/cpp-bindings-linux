cmake_minimum_required(VERSION 3.30)

project(cpp_unix_bindings VERSION 1.0.0 LANGUAGES CXX)

# Ensure position-independent code for all targets (required for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Always use pthread
add_link_options(-pthread)
add_compile_options(-pthread)

# Generate compile_commands.json for clang-based tooling (clangd / clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

# Dependencies
CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
    OPTIONS
    "GTEST_BUILD_TESTS OFF"
    "GTEST_BUILD_EXAMPLES OFF"
    "GTEST_BUILD_DOCS OFF"
)

CPMAddPackage(
    NAME cpp_core
    GITHUB_REPOSITORY Serial-IO/cpp-core
    GIT_TAG revamp-project
)

# Collect all source files
file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Main library
add_library(${PROJECT_NAME} SHARED ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE cpp_core)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Test executable
add_executable(tests
    tests/serial_test.cpp
    tests/serial_unit_tests.cpp
)

target_link_libraries(tests PRIVATE ${PROJECT_NAME} gtest cpp_core)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

# Copy shared library next to test binary
add_custom_command(TARGET tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${PROJECT_NAME}>
    $<TARGET_FILE_DIR:tests>
    COMMENT "Copy shared library next to test binary"
)

# Testing
enable_testing()
add_test(NAME AllTests COMMAND tests /dev/ttyUSB0)
