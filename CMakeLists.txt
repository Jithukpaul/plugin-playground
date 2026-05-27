cmake_minimum_required(VERSION 3.14)
project(plugin_system CXX)

# C++14, no compiler extensions (so we don't accidentally use GCC-isms).
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Hidden visibility by default. This is THE rule that makes our
# PLUGIN_API export macros meaningful: without it, GCC/Clang export
# every extern symbol from a shared library. With it, only symbols
# we explicitly mark are exported. Matches Windows DLL behavior.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Position-independent code is required for shared libraries on Linux
# and harmless on Windows.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Put plugin .so/.dll files next to the host executable so we can find
# them at runtime without messing with rpath / PATH (for now).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Warnings. Loud during development.
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Targets -------------------------------------------------------------

# The contract. INTERFACE library = no compiled output, just propagates
# include path and C++14 requirement to anyone who links it.
add_library(plugin_api INTERFACE)
target_include_directories(plugin_api INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(plugin_api INTERFACE cxx_std_14)

add_subdirectory(plugins/hello)
add_subdirectory(host)