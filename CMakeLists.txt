cmake_minimum_required(VERSION 3.19)
project(dpset LANGUAGES CXX)
set(PROJECT_VERSION "1.0.0")

execute_process(
  COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE BUILD_NUMBER
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)
include_directories(${CMAKE_BINARY_DIR})

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

qt_add_executable(dpset
    main.cpp
    mainwindow.cpp
    mainwindow.h
    monitoritem.cpp
    monitoritem.h
    xrandrbackend.cpp
    xrandrbackend.h
    orientation.h
    xinputbackend.cpp
    xinputbackend.h
)

qt6_add_resources(dpset_RESOURCES
    ${CMAKE_SOURCE_DIR}/resources.qrc
)
target_sources(dpset PRIVATE ${dpset_RESOURCES})

target_link_libraries(dpset
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
