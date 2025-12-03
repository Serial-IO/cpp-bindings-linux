cmake_minimum_required(VERSION 3.30)

# Export compile commands to root directory
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME cmake_git_versioning
    GITHUB_REPOSITORY Katze719/cmake-git-versioning
    GIT_TAG v1.0.0
)

# Include cmake-git-versioning module
include(${cmake_git_versioning_SOURCE_DIR}/cmake/cmake-git-versioning.cmake)

get_git_version_info()

project(
    cpp-bindings-linux
    VERSION "${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH}"
    DESCRIPTION "Linux implementation of cpp-core serial communication bindings"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable C++23 module support
set(CMAKE_CXX_MODULE_STD 23)
set(CMAKE_CXX_MODULE_EXTENSIONS OFF)

CPMAddPackage(
    NAME cpp_core
    GITHUB_REPOSITORY Serial-IO/cpp-core
    GIT_TAG main
    OPTIONS
    "CMAKE_EXPORT_COMPILE_COMMANDS OFF"
)

# Generate version information
generate_git_version(
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
    OUTPUT_FILE version.hpp
)

CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.14.0
    OPTIONS
    "INSTALL_GTEST OFF"
    "gtest_force_shared_crt ON"
)

file(
    GLOB_RECURSE LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(cpp_bindings_linux SHARED ${LIB_SOURCES})

set_target_properties(
    cpp_bindings_linux
    PROPERTIES
    VERSION "${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH}"
    SOVERSION "${GIT_VERSION_MAJOR}"
    OUTPUT_NAME cpp_bindings_linux
)

target_include_directories(
    cpp_bindings_linux
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
    cpp_bindings_linux
    PUBLIC
    cpp_core::cpp_core
)

target_compile_features(cpp_bindings_linux PUBLIC cxx_std_23)


enable_testing()

# Collect all test source files
file(
    GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

if(TEST_SOURCES)
    add_executable(cpp_bindings_linux_tests ${TEST_SOURCES})

    target_include_directories(
        cpp_bindings_linux_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
    )

    target_link_libraries(
        cpp_bindings_linux_tests
        PRIVATE
        cpp_bindings_linux
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
    )

    target_compile_features(cpp_bindings_linux_tests PRIVATE cxx_std_23)

    include(GoogleTest)
    gtest_discover_tests(cpp_bindings_linux_tests)
endif()

include(GNUInstallDirs)

install(
    TARGETS cpp_bindings_linux
    EXPORT cpp_bindings_linuxTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Copy generated version header
install(
    FILES ${CMAKE_BINARY_DIR}/generated/version.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    OPTIONAL
)
