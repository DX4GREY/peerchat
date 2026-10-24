# CMake/Dependencies.cmake

include(FetchContent)
find_package(PkgConfig QUIET)

# 1. Qt 6 Core & Network
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
    Sql
)

if(PEERCHAT_BUILD_CLIENT)
    find_package(Qt6 REQUIRED COMPONENTS
        Gui
        Qml
        Quick
        QuickControls2
    )
endif()

# 2. libsodium
find_package(libsodium CONFIG QUIET)
if(NOT libsodium_FOUND)
    find_package(unofficial-sodium CONFIG QUIET)
    if(TARGET unofficial-sodium::sodium)
        set(libsodium_FOUND TRUE)
        add_library(libsodium::libsodium INTERFACE IMPORTED)
        target_link_libraries(libsodium::libsodium INTERFACE unofficial-sodium::sodium)
    endif()
endif()

if(NOT libsodium_FOUND)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBSODIUM libsodium)
    endif()
    if(NOT LIBSODIUM_FOUND)
        find_path(LIBSODIUM_INCLUDE_DIR sodium.h
            HINTS
                "${LIBSODIUM_DIR}/include"
                ENV LIBSODIUM_DIR
                "C:/vcpkg/installed/x64-windows/include"
        )
        find_library(LIBSODIUM_LIBRARY NAMES sodium libsodium
            HINTS
                "${LIBSODIUM_DIR}/lib"
                ENV LIBSODIUM_DIR
                "C:/vcpkg/installed/x64-windows/lib"
        )
        if(LIBSODIUM_INCLUDE_DIR AND LIBSODIUM_LIBRARY)
            set(LIBSODIUM_FOUND TRUE)
            add_library(libsodium::libsodium UNKNOWN IMPORTED)
            set_target_properties(libsodium::libsodium PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LIBSODIUM_INCLUDE_DIR}"
                IMPORTED_LOCATION "${LIBSODIUM_LIBRARY}"
            )
        else()
            message(FATAL_ERROR "libsodium not found! Please install libsodium development packages.")
        endif()
    else()
        add_library(libsodium::libsodium INTERFACE IMPORTED)
        set_target_properties(libsodium::libsodium PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LIBSODIUM_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${LIBSODIUM_LIBRARIES}"
        )
    endif()
endif()

# 3. SQLite3
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    find_package(unofficial-sqlite3 CONFIG QUIET)
    if(TARGET unofficial-sqlite3::sqlite3)
        set(SQLite3_FOUND TRUE)
        add_library(SQLite3::SQLite3 INTERFACE IMPORTED)
        target_link_libraries(SQLite3::SQLite3 INTERFACE unofficial-sqlite3::sqlite3)
    endif()
endif()
if(NOT SQLite3_FOUND)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SQLITE3 sqlite3)
    endif()
    if(NOT SQLITE3_FOUND)
        find_path(SQLITE3_INCLUDE_DIR sqlite3.h
            HINTS
                "C:/vcpkg/installed/x64-windows/include"
        )
        find_library(SQLITE3_LIBRARY NAMES sqlite3
            HINTS
                "C:/vcpkg/installed/x64-windows/lib"
        )
        if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
            add_library(SQLite3::SQLite3 UNKNOWN IMPORTED)
            set_target_properties(SQLite3::SQLite3 PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}"
                IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
            )
        endif()
    else()
        add_library(SQLite3::SQLite3 INTERFACE IMPORTED)
        set_target_properties(SQLite3::SQLite3 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${SQLITE3_LIBRARIES}"
        )
    endif()
endif()

# 4. GoogleTest (for unit & integration tests)
if(PEERCHAT_BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found system-wide, fetching via FetchContent...")
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()
