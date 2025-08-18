cmake_minimum_required(VERSION 3.16)
project(EmailRemoteControl VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets Network Concurrent REQUIRED)

set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.ui
)

set(HEADERS
    MainWindow.h
    selectremotedialog.h
    gmailaccountdialog.h
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Concurrent
            ws2_32
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/credentials.json"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
