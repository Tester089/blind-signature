cmake_minimum_required(VERSION 3.20)
project(blind_desktop LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(blind_desktop
        main.cpp
        mainwindow.cpp
        mainwindow.h
)
set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(blind_desktop PRIVATE Qt6::Widgets)


target_include_directories(blind_desktop PRIVATE ${CMAKE_SOURCE_DIR})
target_sources(blind_desktop PRIVATE
        ${REPO_ROOT}/core/core.cpp
)
