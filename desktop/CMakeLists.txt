cmake_minimum_required(VERSION 3.20)
project(blind_desktop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

set(APP_ICON_ICO ${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico)
set(APP_ICON_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/assets/app.icns)

if (WIN32)
    set(APP_ICON_RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/assets/app.rc)
endif()

qt_add_executable(blind_desktop
        ${APP_ICON_RESOURCE}
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

set_target_properties(blind_desktop PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE app.icns
)

set_source_files_properties(${APP_ICON_ICNS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
)

target_sources(blind_desktop PRIVATE
        ${APP_ICON_ICNS}
)

target_link_libraries(blind_desktop PRIVATE
        Qt6::Widgets
        Qt6::Network
)

if (MSVC)
    target_compile_options(blind_desktop PRIVATE /W4 /utf-8 /EHsc)
else()
    target_compile_options(blind_desktop PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

target_include_directories(blind_desktop PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${REPO_ROOT}
)

target_sources(blind_desktop PRIVATE
        ${REPO_ROOT}/core/core.cpp
)