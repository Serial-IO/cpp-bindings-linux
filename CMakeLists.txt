cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME cmake_git_versioning
    GITHUB_REPOSITORY Katze719/cmake-git-versioning
    GIT_TAG v1.1.0
)

include(${cmake_git_versioning_SOURCE_DIR}/cmake/cmake-git-versioning.cmake)
get_git_version_info()

project(
    cpp-bindings-linux
    VERSION "${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH}"
    DESCRIPTION "Linux implementation of cpp-core serial communication bindings"
    LANGUAGES CXX
)

include(CTest)

file(WRITE "${CMAKE_BINARY_DIR}/env.sh" "PACKAGE_VERSION=${GIT_DESCRIBE_NO_V}")
generate_git_version(
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
    OUTPUT_FILE version.hpp
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD 26)
set(CMAKE_CXX_MODULE_EXTENSIONS OFF)

option(
    CPP_BINDINGS_LINUX_ENABLE_FFI_JSON_EXPORT
    "Enable ASTrein JSON export for the cpp-core FFI headers"
    OFF
)
set(
    CPP_BINDINGS_LINUX_ASTREIN_EXECUTABLE
    ""
    CACHE FILEPATH
    "Path to the ASTrein executable used for FFI JSON export"
)
set(
    CPP_BINDINGS_LINUX_FFI_JSON_OUTPUT
    "${CMAKE_BINARY_DIR}/ast/cpp_core_ffi_api.json"
    CACHE FILEPATH
    "Output path for the generated cpp-core FFI API metadata"
)

CPMAddPackage(
    NAME cpp_core
    GITHUB_REPOSITORY Serial-IO/cpp-core
    GIT_TAG v2.0.1
    OPTIONS
    "CMAKE_EXPORT_COMPILE_COMMANDS OFF"
)

if(CPP_BINDINGS_LINUX_ENABLE_FFI_JSON_EXPORT)
    if(CPP_BINDINGS_LINUX_ASTREIN_EXECUTABLE)
        set(_cpp_bindings_linux_astrein "${CPP_BINDINGS_LINUX_ASTREIN_EXECUTABLE}")
    else()
        find_program(_cpp_bindings_linux_astrein NAMES astrein)
    endif()

    if(NOT _cpp_bindings_linux_astrein)
        message(
            FATAL_ERROR
            "CPP_BINDINGS_LINUX_ENABLE_FFI_JSON_EXPORT=ON requires ASTrein. "
            "Install astrein or set CPP_BINDINGS_LINUX_ASTREIN_EXECUTABLE."
        )
    endif()

    file(
        GLOB_RECURSE _cpp_bindings_linux_ffi_headers
        CONFIGURE_DEPENDS
        "${cpp_core_SOURCE_DIR}/include/*.h"
        "${cpp_core_SOURCE_DIR}/include/*.hpp"
    )
    set(
        _cpp_bindings_linux_ffi_wrapper
        "${CMAKE_BINARY_DIR}/ast/cpp_core_ffi_api.cpp"
    )
    get_filename_component(
        _cpp_bindings_linux_ffi_output_dir
        "${CPP_BINDINGS_LINUX_FFI_JSON_OUTPUT}"
        DIRECTORY
    )

    file(
        GENERATE
        OUTPUT "${_cpp_bindings_linux_ffi_wrapper}"
        CONTENT "#include <cpp_core/serial.h>\n"
    )

    add_library(
        cpp_bindings_linux_ffi_ast_context
        OBJECT
        EXCLUDE_FROM_ALL
        "${_cpp_bindings_linux_ffi_wrapper}"
    )
    target_include_directories(
        cpp_bindings_linux_ffi_ast_context
        PRIVATE
        "${cpp_core_SOURCE_DIR}/include"
    )
    target_compile_features(
        cpp_bindings_linux_ffi_ast_context
        PRIVATE
        cxx_std_26
    )

    add_custom_command(
        OUTPUT "${CPP_BINDINGS_LINUX_FFI_JSON_OUTPUT}"
        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            "${_cpp_bindings_linux_ffi_output_dir}"
        COMMAND
            "${_cpp_bindings_linux_astrein}"
            --ffi
            --compile-commands "${CMAKE_BINARY_DIR}/compile_commands.json"
            --require-c-linkage
            --require-default-visibility
            --public-header "cpp_core/serial.h"
            --api-root "${cpp_core_SOURCE_DIR}/include"
            --output "${CPP_BINDINGS_LINUX_FFI_JSON_OUTPUT}"
            "${_cpp_bindings_linux_ffi_wrapper}"
        DEPENDS
            "${_cpp_bindings_linux_astrein}"
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${_cpp_bindings_linux_ffi_wrapper}"
            ${_cpp_bindings_linux_ffi_headers}
        COMMENT "Exporting cpp-core FFI API metadata with ASTrein"
        VERBATIM
    )

    add_custom_target(
        cpp_bindings_linux_ffi_json
        DEPENDS "${CPP_BINDINGS_LINUX_FFI_JSON_OUTPUT}"
    )
endif()

CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.14.0
    OPTIONS
    "INSTALL_GTEST OFF"
    "gtest_force_shared_crt ON"
)

file(GLOB_RECURSE LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*\\.test\\.cpp$")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/test_helpers/.*")

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

target_compile_features(cpp_bindings_linux PUBLIC cxx_std_26)

file(GLOB SRC_UNIT_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.test.cpp")
file(GLOB TESTS_INTEGRATION "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp")
file(GLOB TEST_HELPER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/test_helpers/*.cpp")
set(TEST_SOURCES ${SRC_UNIT_TESTS} ${TESTS_INTEGRATION} ${TEST_HELPER_SOURCES})

if(TEST_SOURCES)
    add_executable(cpp_bindings_linux_tests ${TEST_SOURCES})

    target_include_directories(
        cpp_bindings_linux_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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

    target_compile_features(cpp_bindings_linux_tests PRIVATE cxx_std_26)

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

install(
    FILES ${CMAKE_BINARY_DIR}/generated/version.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    OPTIONAL
)

if(CMAKE_EXPORT_COMPILE_COMMANDS AND EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    add_custom_target(
        copy-compile-commands
        ALL
        ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
        DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to project root"
    )
endif()
