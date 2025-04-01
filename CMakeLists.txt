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

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)
qt_standard_project_setup()

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/monitoritem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/monitoritem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/xrandrbackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xrandrbackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/orientation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/xinputbackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xinputbackend.h
)

set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/dpset_nl.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/dpset_en.ts
)

qt_add_executable(dpset
    ${SRC_FILES}
)

qt_add_translations(dpset
    TS_FILES ${TS_FILES}
    SOURCES ${SRC_FILES}
    QM_FILES_OUTPUT_VARIABLE DpsetQmFiles
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

#Todo: Move to workflow
add_dependencies(dpset dpset_lupdate)
add_dependencies(dpset dpset_lrelease)
