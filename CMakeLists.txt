cmake_minimum_required(VERSION 3.14.5)
project(trs)

set (CMAKE_CXX_STANDARD 17)
add_executable(trs main.cpp
    miniz.h
    miniz.c)


option(trs_json_external "Use System/Parent json library" OFF)
if(NOT ${xhfr_json_external})

    set(JSON_BuildTests OFF CACHE INTERNAL "")
  add_subdirectory(json)
endif()


target_include_directories(trs PUBLIC args)
target_link_libraries(trs  PUBLIC nlohmann_json::nlohmann_json)
