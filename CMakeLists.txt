cmake_minimum_required(VERSION 3.13)  # CMake version check
project(MusicDatabaseManager)               # Create project "simple_example"

if(NOT DEFINED TARGET_PLATFORM)
    set(TARGET_PLATFORM "LINUX")
endif()

if(TARGET_PLATFORM STREQUAL "WINDOWS")
    message(STATUS "Compiling for Windows")
    set(CMAKE_FIND_ROOT_PATH /opt/mxe/usr/i686-w64-mingw32.static)
    set(CMAKE_C_COMPILER i686-w64-mingw32.static-gcc)
    set(CMAKE_CXX_COMPILER i686-w64-mingw32.static-g++)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

elseif(TARGET_PLATFORM STREQUAL "LINUX")
    message(STATUS "Compiling for Linux")

else()
    message(STATUS "Unknown TARGET_PLATFORM: ${TARGET_PLATFORM}")

endif()

set(CMAKE_CXX_STANDARD 20)

# Compilation and linker flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_library(common_flags INTERFACE)
target_compile_options(common_flags INTERFACE
    $<$<CONFIG:Debug>: -Wall -Wextra -g -Wall -Wextra -Werror -Wpointer-arith -Wcast-qual -Wno-missing-braces -Wempty-body -Wno-error=uninitialized -Wno-error=deprecated-declarations -pedantic-errors -pedantic -Os>
    $<$<CONFIG:Release>: -O3>)

# binary filename and location
set(EXEC_NAME "bintest")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)


# C++ JSON Library 
find_package(nlohmann_json 3.12.0 REQUIRED)

# QT 5 Library
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Concurrent REQUIRED)

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")
set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")


# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Building
add_executable(${EXEC_NAME} ${SOURCES})
target_include_directories(${EXEC_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(${EXEC_NAME} PRIVATE ${common_flags})

target_link_libraries(${EXEC_NAME} Qt5::Core)
target_link_libraries(${EXEC_NAME} Qt5::Widgets)
target_link_libraries(${EXEC_NAME} Qt5::Concurrent)

if(TARGET_PLATFORM STREQUAL "WINDOWS")
    target_link_libraries(${EXEC_NAME} ws2_32 mswsock)
endif()

