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
    mainwindow.h
    mainwindow.cpp

    pages/dashboard_page.h
    pages/dashboard_page.cpp
    pages/verify_page.h
    pages/verify_page.cpp
    pages/settings_page.h
    pages/settings_page.cpp

    widgets/connect_button.h
    widgets/connect_button.cpp
    widgets/status_badge.h
    widgets/status_badge.cpp
    widgets/card_frame.h
    widgets/card_frame.cpp
    widgets/copy_field.h
    widgets/copy_field.cpp

    net/api_client.h
    net/api_client.cpp
    net/dto.h

    privacy/encryptor.h
    privacy/stub_encryptor.h
    privacy/stub_encryptor.cpp

    outbox/outbox_store.h
    outbox/outbox_store.cpp
    outbox/outbox_worker.h
    outbox/outbox_worker.cpp

    utils/time.h
    utils/time.cpp
)

target_link_libraries(blind_desktop PRIVATE Qt6::Widgets Qt6::Network)


set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

target_include_directories(blind_desktop PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${REPO_ROOT}
)


if (EXISTS "/core/core.cpp")
    target_sources(blind_desktop PRIVATE
        /core/core.cpp
    )
else()
    target_sources(blind_desktop PRIVATE
        core_bridge.cpp
    )
    set_source_files_properties(core_bridge.cpp PROPERTIES COMPILE_DEFINITIONS "BLIND_SIG_IMPLEMENTATION")
endif()
