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
configure_file(${CMAKE_SOURCE_DIR}/version.h.in
               ${CMAKE_BINARY_DIR}/version.h @ONLY)
include_directories(${CMAKE_BINARY_DIR})

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)
qt_standard_project_setup()

get_target_property(LUPDATE_EXE Qt6::lupdate IMPORTED_LOCATION)
if(NOT LUPDATE_EXE)
    message(FATAL_ERROR "Could not find lupdate executable from Qt6::lupdate.")
endif()

file(GLOB TS_FILES "${CMAKE_SOURCE_DIR}/translations/*.ts")

execute_process(
    COMMAND "${LUPDATE_EXE}"
            -locations relative
            -no-obsolete
            "${CMAKE_SOURCE_DIR}"
            -ts ${TS_FILES}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE LUPDATE_RESULT
    OUTPUT_VARIABLE LUPDATE_STDOUT
    ERROR_VARIABLE  LUPDATE_STDERR
)
if(LUPDATE_RESULT)
    message(FATAL_ERROR "lupdate returned an error code ${LUPDATE_RESULT}:\n${LUPDATE_ERROR}")
else()
    message(STATUS "lupdate OK. Output:\n${LUPDATE_OUTPUT}")
endif()

set(SRC_FILES
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

qt_add_executable(dpset
    ${SRC_FILES}
)

qt_add_translations(dpset
    TS_FILES ${TS_FILES}
    SOURCES ${SRC_FILES}
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
