cmake_minimum_required(VERSION 3.20)

project(blind_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(blind_server
        main.cpp
        handlers.cpp
        json_utils.cpp
        poll.cpp
        core/core.cpp
)

target_include_directories(blind_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_compile_definitions(blind_server PRIVATE
        _WIN32_WINNT=0x0A00
)

if (MSVC)
    target_compile_options(blind_server PRIVATE /W4 /utf-8 /EHsc)
else()
    target_compile_options(blind_server PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_link_libraries(blind_server PRIVATE ws2_32)
endif()

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(blind_tests
        tests.cpp
        json_utils.cpp
        poll.cpp
)

target_include_directories(blind_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_compile_definitions(blind_tests PRIVATE
        _WIN32_WINNT=0x0A00
)

if (MSVC)
    target_compile_options(blind_tests PRIVATE /W4 /utf-8 /EHsc)
else()
    target_compile_options(blind_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(blind_tests PRIVATE GTest::gtest_main)

if (WIN32)
    target_link_libraries(blind_tests PRIVATE ws2_32)
endif()

add_test(NAME blind_tests COMMAND blind_tests)
