cmake_minimum_required(VERSION 3.25)

project(frankenquake)

option(USE_SDL "use SDL2" ON)
set(GAME_NAME "quake2")

if (USE_SDL)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIR})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(game/${GAME_NAME})

FILE(GLOB ref_soft_SOURCES render/ref_soft/*.c)
FILE(GLOB ref_gl_SOURCES render/ref_gl/*.c)

add_library(ref_soft ${ref_soft_SOURCES})
target_include_directories(ref_soft PRIVATE render)

