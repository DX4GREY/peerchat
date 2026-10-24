cmake_minimum_required(VERSION 3.22)

project(PeerChat
    VERSION 0.1.0
    DESCRIPTION "Secure cross-platform peer-to-peer messaging and file sharing application"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt MOC, RCC, and UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Export compile commands for clang-tidy / LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Modern compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wno-unused-parameter)
endif()

# Build options
option(PEERCHAT_BUILD_CLIENT "Build Qt6 QML Desktop/Mobile client" ON)
option(PEERCHAT_BUILD_RELAY "Build standalone relay daemon" ON)
option(PEERCHAT_BUILD_TESTS "Build automated test suite" ON)

# Enable testing if requested
if(PEERCHAT_BUILD_TESTS)
    enable_testing()
endif()

# Include third-party dependency resolution
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Dependencies.cmake)

# Subdirectories
add_subdirectory(core)

if(PEERCHAT_BUILD_RELAY)
    add_subdirectory(services/relay)
endif()

if(PEERCHAT_BUILD_CLIENT)
    add_subdirectory(apps/client)
endif()

if(PEERCHAT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
