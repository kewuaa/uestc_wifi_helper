cmake_minimum_required(VERSION 3.15)

project(uestc_wifi_helper LANGUAGES C CXX)

if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    message(FATEL "not support Darwin yet")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAG_DEBUG "-O0 -Wall -g -ggdb")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(cmake/CPM.cmake)
include(GenerateExportHeader)

CPMAddPackage("gh:CLIUtils/CLI11@2.5.0")
CPMAddPackage("gh:nlohmann/json@3.12.0")
CPMAddPackage("gh:Neargye/magic_enum@0.9.7")
CPMAddPackage("gh:yhirose/cpp-httplib@0.26.0")
CPMAddPackage("gh:marzer/tomlplusplus#v3.4.0")
CPMAddPackage(
    NAME cryptopp-cmake
    GIT_REPOSITORY "https://github.com/abdes/cryptopp-cmake.git"
    GIT_TAG CRYPTOPP_8_9_0
    OPTIONS "CRYPTOPP_BUILD_TESTING OFF"
)

find_package(spdlog)
if (NOT spdlog_FOUND)
    message(STATUS "spdlog not found in system, download it by CPM")
    CPMAddPackage(
        NAME spdlog
        GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
        GIT_TAG v1.16.0
        OPTIONS "SPDLOG_USE_STD_FORMAT TRUE"
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    CPMAddPackage("gh:Kistler-Group/sdbus-cpp@2.1.0")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Windows AND MSVC)
    add_compile_options("/Zc:preprocessor")
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

add_library(uestc_wifi_lib)
generate_export_header(
    uestc_wifi_lib
    BASE_NAME uestc_wifi
    EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/uestc_wifi_export.hpp
)
target_compile_options(uestc_wifi_lib PUBLIC -UCPPHTTPLIB_ZLIB_SUPPORT)
target_include_directories(
    uestc_wifi_lib
    PUBLIC
        include
        ${CMAKE_BINARY_DIR}
)
target_sources(
    uestc_wifi_lib
    PRIVATE
        src/uestc_wifi/encrypt.cpp
        src/uestc_wifi/impl.cpp
)
target_link_libraries(
    uestc_wifi_lib
    PUBLIC
        httplib
        spdlog::spdlog
        cryptopp::cryptopp
        magic_enum::magic_enum
        nlohmann_json::nlohmann_json
)
# windows 下 mingw 经测试找不到 GetAddrInfoExCancel
if (CMAKE_SYSTEM_NAME STREQUAL Windows AND MINGW)
    target_compile_definitions(
        uestc_wifi_lib
        PUBLIC -UCPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO
    )
endif()

add_executable(uestc_wifi)
target_sources(uestc_wifi PRIVATE src/uestc_wifi/main.cpp)
target_link_libraries(
    uestc_wifi
    PRIVATE
        CLI11::CLI11
        uestc_wifi_lib
)

add_executable(uestc_wifi_helper)
target_sources(
    uestc_wifi_helper
    PRIVATE
        src/uestc_wifi_helper/config.cpp
        src/uestc_wifi_helper/impl.cpp
        src/uestc_wifi_helper/main.cpp
)
target_link_libraries(
    uestc_wifi_helper
    PRIVATE
        CLI11::CLI11
        uestc_wifi_lib
        tomlplusplus::tomlplusplus
)
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(
        uestc_wifi_helper
        PRIVATE
            src/uestc_wifi_helper/impl_linux.cpp
    )
    target_link_libraries(uestc_wifi_helper PRIVATE sdbus-c++)
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_sources(
        uestc_wifi_helper
        PRIVATE
            src/uestc_wifi_helper/impl_windows.cpp
    )
endif()
