cmake_minimum_required(VERSION 3.20)
project(blind_desktop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

qt_add_executable(blind_desktop WIN32
        main.cpp
        mainwindow.cpp
        mainwindow.h

        pages/dashboard_page.cpp
        pages/dashboard_page.h
        pages/verify_page.cpp
        pages/verify_page.h
        pages/settings_page.cpp
        pages/settings_page.h

        widgets/connect_button.cpp
        widgets/connect_button.h
        widgets/status_badge.cpp
        widgets/status_badge.h
        widgets/card_frame.cpp
        widgets/card_frame.h

        net/api_client.cpp
        net/api_client.h
        net/dto.h
)

target_link_libraries(blind_desktop PRIVATE
        Qt6::Widgets
        Qt6::Network
)

set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

target_include_directories(blind_desktop PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${REPO_ROOT}
)

target_sources(blind_desktop PRIVATE
        ${REPO_ROOT}/core/core.cpp
)