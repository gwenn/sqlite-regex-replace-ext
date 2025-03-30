cmake_minimum_required(VERSION 3.30)
project(regex_replace_ext C)

set(CMAKE_C_STANDARD 11)
set(BASE_COMPILER_OPTIONS "-Wall -Wextra")

find_package(PkgConfig REQUIRED)

pkg_search_module(PKG_GLIB REQUIRED IMPORTED_TARGET glib-2.0)
add_library(glib_replace SHARED glib_replace.c)
target_link_libraries(glib_replace
        PRIVATE
        PkgConfig::PKG_GLIB)
set_property(TARGET glib_replace PROPERTY POSITION_INDEPENDENT_CODE ON)

pkg_search_module(PKG_ICU REQUIRED IMPORTED_TARGET icu-io)
add_library(icu_replace SHARED icu_replace.c)
target_link_libraries(icu_replace
        PRIVATE
        PkgConfig::PKG_ICU)
set_property(TARGET icu_replace PROPERTY POSITION_INDEPENDENT_CODE ON)
