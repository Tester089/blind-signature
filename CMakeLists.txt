cmake_minimum_required(VERSION 3.20)
project(BlindSignature CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(blind_sig main.cpp)
target_include_directories(blind_sig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(blind_sig_tests tests/test002.cpp)
target_include_directories(blind_sig_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(blind_sig_tests gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(blind_sig_tests)